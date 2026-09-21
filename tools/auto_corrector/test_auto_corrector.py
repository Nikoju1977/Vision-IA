import unittest

from auto_corrector import auto_correct, extract_code, run_generated_code


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


if __name__ == "__main__":
    unittest.main()
