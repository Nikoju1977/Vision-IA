import json
import unittest

from auto_corrector import auto_correct, extract_code, run_generated_code
from mistral_provider import MistralProvider, create_mistral_agent
from vision_ia_agent import VisionIAAgent


class AutoCorrectorTests(unittest.TestCase):
    def test_extracts_python_fence(self):
        text = "avant\n" + chr(96) * 3 + "python\nprint(42)\n" + chr(96) * 3 + "\napres"
        self.assertEqual(extract_code(text), "print(42)")

    def test_executes_valid_code(self):
        result = run_generated_code("print(sum(range(10)))", timeout_seconds=2)
        self.assertEqual(result.returncode, 0)
        self.assertIn("45", result.stdout)

    def test_reports_syntax_error_without_running(self):
        result = run_generated_code("def broken(:\n    pass", timeout_seconds=2)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("SyntaxError", result.stderr)

    def test_agent_repairs_failure(self):
        fence = chr(96) * 3
        responses = iter([
            f"{fence}python\nprint(1 / 0)\n{fence}",
            f"{fence}python\nprint(50)\n{fence}",
        ])

        result = auto_correct(
            "Afficher 50",
            lambda _prompt: next(responses),
            max_iterations=3,
            timeout_seconds=2,
        )

        self.assertTrue(result.success)
        self.assertEqual(len(result.attempts), 2)
        self.assertIn("print(50)", result.code or "")


class VisionIAAgentTests(unittest.TestCase):
    def test_strict_json_success(self):
        def provider(system_prompt, prompt):
            self.assertIn("JSON valide", system_prompt)
            self.assertIn("Objectif", prompt)
            return json.dumps({
                "analyse": "Solution triviale.",
                "code": "print(42)",
            })

        agent = VisionIAAgent(provider, max_iterations=2, timeout_sec=2)
        result = agent.solve("Afficher 42")

        self.assertTrue(result.success)
        self.assertEqual(len(result.iterations), 1)
        self.assertIn("print(42)", result.code or "")

    def test_retries_protocol_error_then_succeeds(self):
        responses = iter([
            "pas du json",
            json.dumps({
                "analyse": "Format corrigé.",
                "code": "print('ok')",
            }),
        ])

        agent = VisionIAAgent(
            lambda _system, _prompt: next(responses),
            max_iterations=3,
            timeout_sec=2,
        )

        result = agent.solve("Afficher ok")
        self.assertTrue(result.success)
        self.assertEqual(len(result.iterations), 1)

    def test_repairs_runtime_failure(self):
        responses = iter([
            json.dumps({
                "analyse": "Première tentative.",
                "code": "print(1 / 0)",
            }),
            json.dumps({
                "analyse": "Suppression de la division par zéro.",
                "code": "print(1)",
            }),
        ])

        agent = VisionIAAgent(
            lambda _system, _prompt: next(responses),
            max_iterations=3,
            timeout_sec=2,
        )

        result = agent.solve("Afficher 1")
        self.assertTrue(result.success)
        self.assertEqual(len(result.iterations), 2)
        self.assertIn("print(1)", result.code or "")

    def test_rejects_extra_json_fields(self):
        with self.assertRaises(ValueError):
            VisionIAAgent._parse_strict_json(json.dumps({
                "analyse": "x",
                "code": "print(1)",
                "extra": True,
            }))


class MistralProviderTests(unittest.TestCase):
    def test_json_mode_request(self):
        class FakeMessage:
            content = json.dumps({
                "analyse": "Diagnostic bref.",
                "code": "print(7)",
            })

        class FakeChoice:
            message = FakeMessage()

        class FakeResponse:
            choices = [FakeChoice()]

        class FakeChat:
            def __init__(self):
                self.kwargs = None

            def complete(self, **kwargs):
                self.kwargs = kwargs
                return FakeResponse()

        class FakeClient:
            def __init__(self):
                self.chat = FakeChat()

        client = FakeClient()
        provider = MistralProvider(
            model="mistral-test",
            client=client,
            temperature=0.2,
        )
        raw = provider("SYSTEM", "PROMPT")

        self.assertEqual(json.loads(raw)["code"], "print(7)")
        self.assertEqual(client.chat.kwargs["model"], "mistral-test")
        self.assertEqual(
            client.chat.kwargs["response_format"],
            {"type": "json_object"},
        )
        self.assertEqual(client.chat.kwargs["temperature"], 0.2)
        self.assertEqual(
            client.chat.kwargs["messages"],
            [
                {"role": "system", "content": "SYSTEM"},
                {"role": "user", "content": "PROMPT"},
            ],
        )

    def test_factory_builds_agent_without_network(self):
        class FakeMessage:
            content = json.dumps({
                "analyse": "OK.",
                "code": "print(9)",
            })

        class FakeChoice:
            message = FakeMessage()

        class FakeResponse:
            choices = [FakeChoice()]

        class FakeChat:
            def complete(self, **_kwargs):
                return FakeResponse()

        class FakeClient:
            chat = FakeChat()

        agent = create_mistral_agent(
            model="mistral-test",
            max_iterations=2,
            timeout_sec=2,
            client=FakeClient(),
        )

        result = agent.solve("Afficher 9")
        self.assertTrue(result.success)
        self.assertIn("print(9)", result.code or "")

    def test_rejects_invalid_temperature(self):
        with self.assertRaises(ValueError):
            MistralProvider(
                model="mistral-test",
                client=object(),
                temperature=2.0,
            )


if __name__ == "__main__":
    unittest.main()
