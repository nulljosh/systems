import requests

from .base import BaseLLM, Response


class ArthurLLM(BaseLLM):
    """Arthur LLM integration via Flask HTTP endpoint."""

    def __init__(self, endpoint: str = "http://localhost:5001"):
        self.endpoint = endpoint.rstrip("/")

    def generate(self, messages, tools=None):
        prompt = messages[-1]["content"] if messages else ""
        try:
            resp = requests.post(
                f"{self.endpoint}/api/generate",
                json={"prompt": prompt, "length": 256, "temperature": 0.7},
                timeout=30,
            )
            resp.raise_for_status()
            data = resp.json()
            return Response(text=data.get("text", ""))
        except requests.ConnectionError:
            return Response(text="[Arthur endpoint not reachable at {}]".format(self.endpoint))
        except requests.RequestException as e:
            return Response(text="[Arthur error: {}]".format(e))
