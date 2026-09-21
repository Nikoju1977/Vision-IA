import json
import tempfile
import unittest
from pathlib import Path

from long_document_processor import (
    MistralLongDocumentProcessor,
    load_document_text,
    split_long_text,
)
from mistral_provider import MistralProvider


class LongDocumentProcessorTests(unittest.TestCase):
    def test_split_long_text_bounds_chunks(self):
        text = ("Paragraphe alpha. " * 1200) + "\n\n" + ("Paragraphe beta. " * 1200)

        chunks = split_long_text(
            text,
            chunk_chars=4000,
            overlap_chars=200,
        )

        self.assertGreater(len(chunks), 2)
        self.assertTrue(all(len(chunk) <= 4000 for chunk in chunks))
        self.assertTrue(all(chunk.strip() for chunk in chunks))

    def test_hierarchical_analysis_never_sends_whole_document(self):
        prompts = []

        def provider(_system, prompt):
            prompts.append(prompt)
            return json.dumps({
                "resume": "Résumé contrôlé.",
                "faits": ["fait A", "fait B"],
                "anomalies": [],
                "references": ["fragment"],
            })

        processor = MistralLongDocumentProcessor(
            provider,
            chunk_chars=2500,
            overlap_chars=100,
            reduction_batch_size=3,
        )

        text = "Texte long. " * 3000
        result = processor.analyze_text(
            text,
            objective="Dépouillement intégral",
        )

        self.assertGreater(result.chunks_processed, 1)
        self.assertGreater(result.provider_calls, result.chunks_processed)
        self.assertGreaterEqual(result.reduction_rounds, 1)
        self.assertTrue(all(len(prompt) < len(text) for prompt in prompts))
        self.assertEqual(result.digest.resume, "Résumé contrôlé.")

    def test_load_text_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "document.txt"
            path.write_text("bonjour", encoding="utf-8")
            self.assertEqual(load_document_text(path), "bonjour")


class MistralAntiSaturationTests(unittest.TestCase):
    def test_retries_retryable_error(self):
        calls = []
        sleeps = []

        class RetryableError(RuntimeError):
            status_code = 429

        class FakeMessage:
            content = json.dumps({
                "analyse": "OK",
                "code": "print(1)",
            })

        class FakeChoice:
            message = FakeMessage()

        class FakeResponse:
            choices = [FakeChoice()]

        class FakeChat:
            def complete(self, **kwargs):
                calls.append(kwargs)
                if len(calls) < 3:
                    raise RetryableError("rate limit")
                return FakeResponse()

        class FakeClient:
            chat = FakeChat()

        clock = [0.0]

        def sleep_fn(seconds):
            sleeps.append(seconds)
            clock[0] += seconds

        provider = MistralProvider(
            model="mistral-test",
            client=FakeClient(),
            max_retries=3,
            base_retry_delay=0.1,
            max_retry_delay=1.0,
            min_interval=0.0,
            sleep_fn=sleep_fn,
            monotonic_fn=lambda: clock[0],
            random_fn=lambda: 0.5,
        )

        output = provider("SYSTEM", "PROMPT")

        self.assertEqual(json.loads(output)["code"], "print(1)")
        self.assertEqual(len(calls), 3)
        self.assertEqual(len(sleeps), 2)
        self.assertGreater(sleeps[1], sleeps[0])

    def test_does_not_retry_non_transient_error(self):
        calls = []

        class FakeChat:
            def complete(self, **kwargs):
                calls.append(kwargs)
                raise ValueError("invalid request")

        class FakeClient:
            chat = FakeChat()

        provider = MistralProvider(
            model="mistral-test",
            client=FakeClient(),
            max_retries=5,
            min_interval=0.0,
            sleep_fn=lambda _seconds: None,
        )

        with self.assertRaises(ValueError):
            provider("SYSTEM", "PROMPT")

        self.assertEqual(len(calls), 1)

    def test_compacts_oversized_prompt_before_api_call(self):
        captured = {}

        class FakeMessage:
            content = json.dumps({
                "analyse": "OK",
                "code": "print(1)",
            })

        class FakeChoice:
            message = FakeMessage()

        class FakeResponse:
            choices = [FakeChoice()]

        class FakeChat:
            def complete(self, **kwargs):
                captured.update(kwargs)
                return FakeResponse()

        class FakeClient:
            chat = FakeChat()

        provider = MistralProvider(
            model="mistral-test",
            client=FakeClient(),
            max_input_chars=4000,
            min_interval=0.0,
        )

        provider(
            "SYSTEM",
            "A" * 12000,
        )

        sent_prompt = captured["messages"][1]["content"]

        self.assertLessEqual(
            len("SYSTEM") + len(sent_prompt),
            4000,
        )
        self.assertIn("CONTEXTE CENTRAL COMPACTÉ", sent_prompt)
        self.assertTrue(sent_prompt.startswith("A"))
        self.assertTrue(sent_prompt.endswith("A"))


if __name__ == "__main__":
    unittest.main()
