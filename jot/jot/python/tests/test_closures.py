import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from python.interpreter import Interpreter


def test_lexical_closure_captures_outer_variable():
    interp = Interpreter()
    result = interp.run(
        """
        fn outer(x) {
            fn inner(y) {
                return x + y;
            }
            return inner;
        }

        let add10 = outer(10);
        add10(5);
        """
    )
    assert result == 15


def test_closure_has_independent_state_per_factory_call():
    interp = Interpreter()
    result = interp.run(
        """
        fn makeCounter(start) {
            let count = start;
            fn next() {
                count += 1;
                return count;
            }
            return next;
        }

        let a = makeCounter(0);
        let b = makeCounter(10);
        a();
        a();
        b();
        a();
        """
    )
    assert result == 3


def test_function_value_can_be_passed_as_argument():
    interp = Interpreter()
    result = interp.run(
        """
        fn applyTwice(f, x) {
            return f(f(x));
        }

        fn inc(n) {
            return n + 1;
        }

        applyTwice(inc, 5);
        """
    )
    assert result == 7
