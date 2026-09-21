import json
import unittest

from auto_corrector import auto_correct, extract_code, run_generated_code
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


if __name__ == "__main__":
    unittest.main()
