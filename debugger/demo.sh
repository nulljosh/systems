#!/bin/bash
cd ~/Documents/Code/debugger

echo "=== DEBUGGER DEMO ==="
echo
echo "1. First, let's see what we're debugging:"
echo
cat test.c
echo
echo "2. Get the main function address:"
nm test_prog | grep main
echo
echo "3. Now run the debugger with commands:"
echo

# Create demo input
cat > demo_input.txt << 'EOF'
run
break 0x100000488
continue
print $rax
step
step
step
continue
quit
EOF

echo "Commands we'll send:"
cat demo_input.txt
echo
echo "=== RUNNING DEBUGGER ==="
./debugger ./test_prog < demo_input.txt