import sys
from prompt_toolkit import PromptSession
from prompt_toolkit.history import FileHistory
from pathlib import Path
from rich.console import Console
from rich.text import Text

from . import __version__
from .agent import Agent
from .config import Config
from .llm.arthur import ArthurLLM


def print_banner(console: Console):
    banner = Text()
    banner.append("piper", style="bold green")
    banner.append(f" v{__version__}", style="dim")
    banner.append(" -- coding assistant powered by Arthur LLM", style="dim")
    console.print(banner)
    console.print("[dim]/quit to exit[/dim]\n")


def main():
    console = Console()
    config = Config.load()

    print_banner(console)

    llm = ArthurLLM(endpoint=config.arthur_endpoint)
    agent = Agent(llm)

    history_path = Path.home() / ".piper" / "history"
    history_path.parent.mkdir(parents=True, exist_ok=True)
    session = PromptSession(history=FileHistory(str(history_path)))

    while True:
        try:
            user_input = session.prompt("piper> ")
        except (EOFError, KeyboardInterrupt):
            break

        user_input = user_input.strip()
        if not user_input:
            continue
        if user_input in ("/quit", "/exit"):
            break

        response = agent.process(user_input)
        console.print(response)
        console.print()


if __name__ == "__main__":
    main()
