"""CLI entry point."""
import sys

def main():
    if len(sys.argv) < 2:
        print("Usage: python -m visual_recognition <train|classify> [args]")
        sys.exit(1)
    print(f"Command: {sys.argv[1]}")
    # TODO: implement

if __name__ == "__main__":
    main()
