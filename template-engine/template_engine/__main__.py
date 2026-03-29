"""CLI entry point."""
import sys

def main():
    if len(sys.argv) < 3:
        print("Usage: python -m template_engine render <template> [data.json]")
        sys.exit(1)
    print(f"Rendering: {sys.argv[2]}")
    # TODO: implement

if __name__ == "__main__":
    main()
