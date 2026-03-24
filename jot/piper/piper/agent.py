from .llm.base import BaseLLM
from .tools import get_all_tools, get_tool


MAX_ITERATIONS = 10
MAX_HISTORY = 200


class Agent:
    def __init__(self, llm: BaseLLM):
        self.llm = llm
        self.tools = get_all_tools()
        self.messages: list[dict] = []

    def _tool_schemas(self) -> list[dict]:
        return [t.to_schema() for t in self.tools]

    def process(self, user_input: str) -> str:
        self.messages.append({"role": "user", "content": user_input})

        for _ in range(MAX_ITERATIONS):
            response = self.llm.generate(self.messages, self._tool_schemas())

            if not response.tool_calls:
                self.messages.append({"role": "assistant", "content": response.text})
                return response.text

            # Execute tool calls
            tool_results = []
            for call in response.tool_calls:
                tool = get_tool(call.name)
                if tool:
                    result = tool.execute(call.arguments)
                else:
                    result = f"[unknown tool: {call.name}]"
                tool_results.append({"tool": call.name, "result": result})

            self.messages.append({
                "role": "assistant",
                "content": response.text,
                "tool_calls": [{"name": c.name, "arguments": c.arguments} for c in response.tool_calls],
            })
            self.messages.append({"role": "tool", "content": str(tool_results)})

        return "[max iterations reached]"
