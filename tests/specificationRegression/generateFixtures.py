#!/usr/bin/env python3
"""AI-generated fixture data. Expected results come from the specification.

Prompt: Create inspectable IFJ26 inputs, runtime stdin, expected program stdout,
compiler/runtime statuses and coverage notes for base, FUNEXP, BOOLTHEN,
CONTROL, ALGEBDT, LISTS and their meaningful combinations. Do not change the
compiler or derive expected results from its current implementation.
"""

from pathlib import Path
import json
import textwrap

HERE = Path(__file__).resolve().parent
CASES = []
HEADER = "// AI-generated regression fixture; specification references and prompt are in README.md.\n"


def clean(text):
    return textwrap.dedent(text).strip("\n")


def program(body, definitions="", declarations="", indent="    ", entry="main"):
    parts = ["import ifj26.*"]
    if declarations:
        parts.append(clean(declarations))
    if definitions:
        parts.append(clean(definitions))
    parts.append(f"@main\ndef {entry}(): Unit =\n" + textwrap.indent(clean(body), indent))
    return "\n\n".join(parts) + "\n"


def printed(expression, double=False):
    body = f"val result = {expression}\n"
    if double:
        body += "val text = toStr(result)\nprint(text)\n"
    else:
        body += "print(result)\n"
    return body + 'print("\\n")'


def add(group, name, body=None, expected="", stdin="", compiler=0, runtime=0,
        definitions="", declarations="", source=None, requires=(), spec="", description=""):
    case_id = group + "_" + name
    if any(case["id"] == case_id for case in CASES):
        raise ValueError("Duplicate fixture: " + case_id)
    if source is None:
        source = program(body or "()", definitions, declarations)
    CASES.append(dict(id=case_id, group=group, description=description or name.replace("_", " "),
                      requires=list(requires), spec=spec, source_text=HEADER + source,
                      stdin_text=stdin, stdout_text=expected, compiler_status=compiler,
                      runtime_status=runtime if compiler == 0 else None))


def base(name, body=None, expected="", **kwargs):
    add("base", name, body, expected, spec=kwargs.pop("spec", "§1–4"), **kwargs)


base("hello", 'print("Hello, IFJ26!\\n")', "Hello, IFJ26!\n")
base("empty_main", "()")
base("inline_main", source="import ifj26.*\n@main def run(): Unit = print(\"inline\\n\")\n", expected="inline\n")
base("unit_value", 'val u: Unit = ()\nprint(u)\nprint("\\n")', "()\n")
base("unit_parameter", 'val u = ()\nval n = consume(u)\nprint(n)\nprint("\\n")', "7\n",
     definitions="def consume(u: Unit): Int = 7")
base("unit_nested_parentheses", 'val u = (((())))\nprint(u)\nprint("\\n")', "()\n")
base("semicolon_separators", 'var x = 1; x = x + 2; print(x); print("\\n")', "3\n")
base("shadowing", '''val x = 1
if x == 1 then
    val x = 9
    print(x)
else
    print(0)
print(x)
print("\\n")''', "91\n")
base("while_sum", '''var i = 1
var sum = 0
while i <= 5 do
    sum = sum + i
    i = i + 1
print(sum)
print("\\n")''', "15\n")
base("while_zero_iterations", '''var x = 0
while x > 0 do
    print("BAD")
print("OK\\n")''', "OK\n")
base("arbitrary_indentation", source='import ifj26.*\n@main\ndef main(): Unit =\n  val x = 1\n  if x == 1 then\n      print("yes\\n")\n  else\n      print("no\\n")\n', expected="yes\n")
base("blank_and_comment_lines", source='import ifj26.*\n@main\ndef main(): Unit =\n    val x = 4\n\t \n  // comment-only indentation is ignored\n    /* comment */\n    print(x)\n    print("\\n")\n', expected="4\n")
base("inline_tab", source='import ifj26.*\n@main\ndef main(): Unit =\n    val\tx = 3\n    print(x)\n    print("\\n")\n', expected="3\n")
base("crlf", source=program('print("CRLF\\n")').replace("\n", "\r\n"), expected="CRLF\n")
base("eof_without_newline", source=program('print("EOF\\n")').rstrip("\n"), expected="EOF\n")
base("line_comment_eof", source=program('print("done\\n") // trailing comment').rstrip("\n"), expected="done\n")
base("nested_comments", '/* outer /* inner */ outer */\nprint("OK\\n")', "OK\n")
base("comment_as_separator", source='import/* separator */ifj26.*\n@main\ndef main(): Unit =\n    val/* comment */x = 5\n    print(x)\n    print("\\n")\n', expected="5\n")
base("block_comment_newline", 'val x = 2 /* outer\n    /* inner */\n    end */\nprint(x)\nprint("\\n")', "2\n")
base("long_identifier", f'val {"identifier_" + "a" * 300} = 17\nprint({"identifier_" + "a" * 300})\nprint("\\n")', "17\n")
base("underscore_identifiers", 'val __ = 1\nval _a2 = 2\nval A_a9 = 3\nval sum = __ + _a2 + A_a9\nprint(sum)\nprint("\\n")', "6\n")
base("contextual_words_are_identifiers", 'val to = 1\nval until = 2\nval by = 3\nval sum = to + until + by\nprint(sum)\nprint("\\n")', "6\n",
     description="to/until/by remain usable as variable names with CONTROL enabled")
base("case_sensitive_identifiers", 'val x = 1\nval X = 2\nval sum = x + X\nprint(sum)\nprint("\\n")', "3\n")

for name, expression, expected in [
    ("precedence", "2 + 3 * 4", 14),
    ("parentheses", "(2 + 3) * 4", 20),
    ("left_associative_subtraction", "20 - 5 - 3", 12),
    ("left_associative_division", "20 / 2 / 2", 5),
    ("division_truncates_to_zero", "(0 - 7) / 2", -3),
    ("division_negative_divisor", "7 / (0 - 2)", -3),
    ("integer_separators", "1___000 + 2__0", 1020),
    ("negative_zero", "-0", 0),
    ("unary_minus", "-(2 + 3) * 4", -20),
    ("zero", "0", 0),
]:
    base(name, printed(expression), f"{expected}\n", spec="§1.1, §3; numeric-separator forum clarification")

for name, expression, expected in [
    ("double_fraction", "12.5", "12.50"),
    ("double_without_integer_part", ".5", "0.50"),
    ("double_exponent", "2e3", "2000.00"),
    ("double_negative_exponent", "2e-1", "0.20"),
    ("double_exponent_leading_zero", "2e001", "20.00"),
    ("double_negative_exponent_leading_zero", "2e-001", "0.20"),
    ("double_repeated_separators", "1__2.5__0e0__0D", "12.50"),
    ("double_leading_zero", "00.25", "0.25"),
    ("double_suffix", "4.0d", "4.00"),
    ("mixed_numeric_arithmetic", "1 + 2.5", "3.50"),
    ("double_division", "7 / 2.0", "3.50"),
]:
    base(name, printed(expression, double=True), expected + "\n", spec="§1.1, §1.3, §3–4")

base("int_to_double_assignment", 'val x: Double = 7\nval text = toStr(x)\nprint(text)\nprint("\\n")', "7.00\n")
base("int_to_double_argument", 'val x = echo(3)\nval text = toStr(x)\nprint(text)\nprint("\\n")', "3.00\n",
     definitions="def echo(x: Double): Double = x")
base("int_to_double_return", 'val x = value()\nval text = toStr(x)\nprint(text)\nprint("\\n")', "4.00\n",
     definitions="def value(): Double = 4")
base("to_int_truncation", 'val a = toInt(3.9)\nval b = toInt(-3.9)\nprint(a)\nprint(",")\nprint(b)\nprint("\\n")', "3,-3\n")
base("string_concat", printed('"ab" + "" + "cd"'), "abcd\n")
base("string_escapes", r'print("A\nB\t\"\\\rZ")', 'A\nB\t"\\\rZ')
base("string_empty", 'val text = ""\nval size = strLength(text)\nprint(size)\nprint("\\n")', "0\n")
base("string_long", printed('"' + "ab" * 600 + '"'), "ab" * 600 + "\n")
base("raw_string", source='import ifj26.*\n@main\ndef main(): Unit =\n    val text = """A\nB\\nC"""\n    print(text)\n    print("\\n")\n', expected="A\nB\\nC\n")
base("raw_quotes_and_comment_text", source='import ifj26.*\n@main\ndef main(): Unit =\n    val text = """one " two "" /* data */ // data"""\n    print(text)\n    print("\\n")\n', expected='one " two "" /* data */ // data\n')
base("string_builtin_boundaries", '''val s = "abcd"
val empty = subStr(s, 1, 1)
val suffix = subStr(s, 1, 4)
val length = strLength(empty)
val first = ord(s, 0)
val bad1 = ord(s, -1)
val bad2 = ord(s, 4)
val bad3 = ord("", 0)
val letter = chr(65)
print(length)
print("|")
print(suffix)
print("|")
print(first)
print("|")
print(bad1)
print("|")
print(bad2)
print("|")
print(bad3)
print("|")
print(letter)
print("\\n")''', "0|bcd|97|-1|-1|-1|A\n")
base("string_comparison", '''if "a" < "b" then
    print("1")
else
    print("0")
if "ab" == "ab" then
    print("1")
else
    print("0")
if "ab" != "a" then
    print("1")
else
    print("0")
print("\\n")''', "111\n")
base("numeric_comparison", '''if 2 < 2.5 then
    print("1")
else
    print("0")
if 2 == 2.0 then
    print("1")
else
    print("0")
if 3 >= 3 then
    print("1")
else
    print("0")
print("\\n")''', "111\n")
base("continuation_logical_baseline", source='import ifj26.*\n@main\ndef main(): Unit =\n    val x = (1 +\n        (2 +\n      3))\n    print(x)\n    print("\\n")\n', expected="6\n")
base("trailing_comma_arguments", source=program('val x = add(\n    2,\n    3,\n    )\nprint(x)\nprint("\\n")',
     definitions="def add(x: Int, y: Int): Int = x + y"), expected="5\n", spec="§1.2")
base("trailing_comma_parameters", 'val x = add(2, 3)\nprint(x)\nprint("\\n")', "5\n",
     definitions="def add(\n    x: Int,\n    y: Int,\n    ): Int = x + y", spec="§1.2")
base("call_before_definition", source='import ifj26.*\n@main\ndef main(): Unit =\n    val x = later(4)\n    print(x)\n    print("\\n")\ndef later(x: Int): Int = x * 2\n', expected="8\n")
base("return_from_nested_block", 'val x = choose(1)\nprint(x)\nprint("\\n")', "7\n",
     definitions='def choose(x: Int): Int =\n    while x > 0 do\n        if x == 1 then\n            return 7\n        else\n            return 8\n    return 0')
base("explicit_unit_discards_value", 'val u = effect()\nprint(u)\nprint("\\n")', "side()\n",
     definitions='def effect(): Unit =\n    print("side")\n    return 123')
base("function_result_last_expression", 'val x = value(4)\nprint(x)\nprint("\\n")', "5\n",
     definitions="def value(x: Int) =\n    val y = x + 1\n    y")
base("while_block_returns_unit", 'val u = effect()\nprint(u)\nprint("\\n")', "()\n",
     definitions="def effect() =\n    var x = 0\n    while x < 2 do\n        x = x + 1")
base("recursive_factorial", 'val x = fact(5)\nprint(x)\nprint("\\n")', "120\n",
     definitions="def fact(n: Int): Int =\n    if n <= 1 then\n        return 1\n    else\n        val prev = n - 1\n        val rest = fact(prev)\n        return n * rest")
base("mutual_recursion", 'val x = even(8)\nprint(x)\nprint("\\n")', "1\n",
     definitions="def even(n: Int): Int =\n    if n == 0 then\n        return 1\n    else\n        val prev = n - 1\n        return odd(prev)\n\ndef odd(n: Int): Int =\n    if n == 0 then\n        return 0\n    else\n        val prev = n - 1\n        return even(prev)")
base("read_line", 'val a = readLine()\nval b = readLine()\nval c = readLine()\nval size = strLength(c)\nprint(a)\nprint(b)\nprint(size)\nprint("\\n")',
     "alpha\nbeta\n0\n", stdin="alpha\nbeta\n")
base("read_line_without_final_lf", 'val a = readLine()\nprint(a)\nprint("|")', "tail|", stdin="tail")
for name, data, expected in [
    ("read_int_valid", "-42\n", "-42\n"),
    ("read_int_invalid", "abc\n", "0\n"),
    ("read_int_whitespace", " 42 \n", "0\n"),
    ("read_int_underscore", "1_000\n", "0\n"),
    ("read_int_eof", "", "0\n"),
]:
    base(name, 'val n = readInt()\nprint(n)\nprint("\\n")', expected, stdin=data, spec="§4")
for name, data, expected in [
    ("read_double_valid", "2.5\n", "2.50\n"),
    ("read_double_invalid", "abc\n", "0.00\n"),
    ("read_double_whitespace", " 2.5\n", "0.00\n"),
    ("read_double_eof", "", "0.00\n"),
]:
    base(name, 'val n = readDouble()\nval text = toStr(n)\nprint(text)\nprint("\\n")', expected, stdin=data, spec="§4")

FX = ("FUNEXP",)
def fx(name, body, expected, **kwargs):
    add("funexp", name, body, expected, requires=FX, spec="§5.1", **kwargs)

fx("nested_calls", 'print(add(twice(3), add(4, 5)))\nprint("\\n")', "15\n",
   definitions="def twice(x: Int): Int = x * 2\ndef add(x: Int, y: Int): Int = x + y")
fx("calls_in_arithmetic", 'val x = 2 * square(3) + square(2)\nprint(x)\nprint("\\n")', "22\n",
   definitions="def square(x: Int): Int = x * x")
fx("arbitrary_argument_expression", 'print(sum(2 + 3 * 4, (8 - 2) / 3))\nprint("\\n")', "16\n",
   definitions="def sum(x: Int, y: Int): Int = x + y")
fx("argument_order", 'print(sum(mark(1), mark(2)))\nprint("\\n")', "123\n",
   definitions="def mark(x: Int): Int =\n    print(x)\n    x\n\ndef sum(a: Int, b: Int): Int = a + b")
fx("operand_order", 'val x = mark(1) + mark(2) * mark(3)\nprint("=")\nprint(x)\nprint("\\n")', "123=7\n",
   definitions="def mark(x: Int): Int =\n    print(x)\n    x")
fx("nested_argument_order", 'val x = sum(mark(1), sum(mark(2), mark(3)))\nprint("=")\nprint(x)\nprint("\\n")', "123=6\n",
   definitions="def mark(x: Int): Int =\n    print(x)\n    x\n\ndef sum(a: Int, b: Int): Int = a + b")
fx("recursive_expression", 'print(fib(10))\nprint("\\n")', "55\n",
   definitions="def fib(n: Int): Int =\n    if n <= 1 then\n        return n\n    else\n        return fib(n - 1) + fib(n - 2)")
fx("builtins_nested", 'print(toStr(strLength(subStr("abcdef", 1 + 1, 6 - 1))))\nprint("\\n")', "3\n")
fx("call_in_while_condition", 'var n = 0\nwhile less(n, 3) do\n    print(n)\n    n = n + 1\nprint("\\n")', "012\n",
   definitions="def less(a: Int, b: Int): Boolean = a < b")
CASES[-1]["requires"].append("BOOLTHEN")
fx("call_in_return", 'print(twice(3))\nprint("\\n")', "10\n",
   definitions="def inc(x: Int): Int = x + 1\ndef twice(x: Int): Int =\n    return inc(inc(x)) * 2")
fx("frame_values_survive_calls", 'print(combine(4))\nprint("\\n")', "15\n",
   definitions="def inc(x: Int): Int = x + 1\ndef combine(x: Int): Int =\n    val left = inc(x)\n    val right = inc(x + 5)\n    left + right")

BT = ("BOOLTHEN",)
def bt(name, body, expected, **kwargs):
    add("boolthen", name, body, expected, requires=BT, spec="§5.2", **kwargs)

for name, expression, expected in [
    ("true_literal", "true", "true"),
    ("false_literal", "false", "false"),
    ("negation", "!false", "true"),
    ("double_negation", "!!true", "true"),
    ("logical_precedence", "true || false && false", "true"),
    ("logical_parentheses", "(true || false) && false", "false"),
    ("relational_precedence", "1 < 2 && 3 >= 3", "true"),
    ("boolean_equality", "true == !false", "true"),
    ("boolean_inequality", "true != false", "true"),
]:
    bt(name, printed(expression), expected + "\n")
for op in ("&&", "||"):
    for left in (False, True):
        for right in (False, True):
            result = left and right if op == "&&" else left or right
            bt(("and" if op == "&&" else "or") + "_" + str(int(left)) + str(int(right)),
               printed(f"{str(left).lower()} {op} {str(right).lower()}"), str(result).lower() + "\n")
bt("boolean_parameter_return", 'val value = invert(true)\nprint(value)\nprint("\\n")', "false\n",
   definitions="def invert(value: Boolean): Boolean = !value")
bt("boolean_variable_assignment", 'var value: Boolean = false\nvalue = !value\nprint(value)\nprint("\\n")', "true\n")
bt("boolean_to_str", 'val text = toStr(false)\nprint(text)\nprint("\\n")', "false\n")
bt("if_without_else", 'if false then\n    print("BAD")\nif true then\n    print("OK\\n")', "OK\n")
bt("else_if", 'val n = 0\nif n < 0 then\n    print("negative\\n")\nelse if n == 0 then\n    print("zero\\n")\nelse\n    print("positive\\n")', "zero\n")
bt("boolean_while", 'var more = true\nvar n = 0\nwhile more do\n    n = n + 1\n    more = n < 3\nprint(n)\nprint("\\n")', "3\n")
bt("read_boolean", 'val a = readBoolean()\nval b = readBoolean()\nprint(a)\nprint(",")\nprint(b)\nprint("\\n")',
   "true,false\n", stdin="true\nfalse\n")
for name, expression, expected in [
    ("short_and_skips", "false && effect()", "false\n"),
    ("short_or_skips", "true || effect()", "true\n"),
    ("short_and_runs", "true && effect()", "Etrue\n"),
    ("short_or_runs", "false || effect()", "Etrue\n"),
]:
    bt(name, printed(expression), expected, definitions='def effect(): Boolean =\n    print("E")\n    true')
    CASES[-1]["requires"].append("FUNEXP")
bt("short_circuit_avoids_division", 'val x = 0\nval value = false && (10 / x > 0)\nprint(value)\nprint("\\n")', "false\n")
bt("short_circuit_still_type_checks", 'val value = false && 1\nprint(value)', "", compiler=6)

CT = ("BOOLTHEN", "CONTROL")
def ct(name, body, expected="", **kwargs):
    add("control", name, body, expected, requires=CT, spec="§5.3", **kwargs)

ct("if_expression_true", printed('if true then 10 else 20'), "10\n")
ct("if_expression_false", printed('if false then 10 else 20'), "20\n")
ct("if_expression_nested_else", printed('if false then 1 else if true then 2 else 3'), "2\n")
ct("if_expression_mixed_numeric", 'val x: Double = if true then 1 else 2.5\nval text = toStr(x)\nprint(text)\nprint("\\n")', "1.00\n")
ct("if_expression_multiline", 'val x = if false then\n    10\nelse\n    20\nprint(x)\nprint("\\n")', "20\n")
ct("if_expression_mixed_layout", 'val x = if false then 10 else\n    20\nprint(x)\nprint("\\n")', "20\n")
ct("if_inline_statement", 'if true then print("YES\\n") else print("NO\\n")', "YES\n")
ct("if_only_selected_branch", 'val x = if true then good() else bad()\nprint(x)\nprint("\\n")', "G7\n",
   definitions='def good(): Int =\n    print("G")\n    7\n\ndef bad(): Int =\n    print("BAD")\n    9')
CASES[-1]["requires"].append("FUNEXP")
for name, header, expected in [
    ("for_inclusive", "i <- 1 to 4", "1234\n"),
    ("for_exclusive", "i <- 1 until 4", "123\n"),
    ("for_step", "i <- 1 to 6 by 2", "135\n"),
    ("for_descending", "i <- 5 to 1 by -2", "531\n"),
    ("for_single_inclusive", "i <- 3 to 3", "3\n"),
    ("for_empty_exclusive", "i <- 3 until 3", "\n"),
    ("for_wrong_direction_empty", "i <- 3 to 1", "\n"),
    ("for_negative_bounds", "i <- -2 to 1", "-2-101\n"),
    ("for_guard", "i <- 1 to 5 if i > 2", "345\n"),
    ("for_two_guards", "i <- 1 to 6 if i > 1 if i < 5", "234\n"),
]:
    ct(name, f'for {header} do\n    print(i)\nprint("\\n")', expected)
ct("for_inline_body", 'for i <- 1 to 3 do print(i)\nprint("\\n")', "123\n")
ct("for_multiline_header", 'for\n    i <- 0 until 5\n    if i != 2\n    if i < 4\ndo\n    print(i)\nprint("\\n")', "013\n")
ct("for_shadowing", 'val i = 99\nfor i <- 1 to 2 do\n    print(i)\nprint(i)\nprint("\\n")', "1299\n")
ct("for_nested", 'for i <- 1 to 2 do\n    for j <- 1 to 2 do\n        print(i)\n        print(j)\nprint("\\n")', "11122122\n")
ct("for_bounds_evaluated_once", 'for i <- mark(1) to mark(3) by mark(1) do\n    print(i)\nprint("\\n")', "131123\n",
   definitions="def mark(x: Int): Int =\n    print(x)\n    x")
CASES[-1]["requires"].append("FUNEXP")
ct("for_guard_order", 'for i <- 1 to 3 if i > 1 if check(i) do\n    print("B")\n    print(i)\nprint("\\n")', "G2B2G3B3\n",
   definitions='def check(x: Int): Boolean =\n    print("G")\n    print(x)\n    true')
CASES[-1]["requires"].append("FUNEXP")
ct("return_from_for", 'val x = find()\nprint(x)\nprint("\\n")', "3\n",
   definitions="def find(): Int =\n    for i <- 1 to 5 do\n        if i == 3 then\n            return i\n    return 0")
ct("if_expression_missing_else", 'val x = if true then 1\nprint(x)', compiler=2)
ct("if_expression_incompatible_branches", 'val x = if true then 1 else "x"\nprint(x)', compiler=6)
ct("for_bound_type", 'for i <- "one" to 3 do\n    print(i)', compiler=6)
ct("for_step_type", 'for i <- 1 to 3 by 1.5 do\n    print(i)', compiler=6)
ct("for_guard_type", 'for i <- 1 to 3 if 1 do\n    print(i)', compiler=6)
ct("for_immutable_binding", 'for i <- 1 to 3 do\n    i = 9', compiler=7)
ct("for_binding_outside_scope", 'for i <- 1 to 2 do\n    print(i)\nprint(i)', compiler=3)
ct("for_literal_zero_step", 'for i <- 1 to 3 by 0 do\n    print(i)', compiler=10)
ct("for_runtime_zero_step", 'val step = readInt()\nprint("before\\n")\nfor i <- 1 to 3 by step do\n    print(i)', "before\n",
   stdin="0\n", runtime=40)

AG = ("ALGEBDT",)
def ag(name, body=None, expected="", **kwargs):
    add("algebdt", name, body, expected, requires=AG, spec=kwargs.pop("spec", "§5.5"), **kwargs)

SCALAR = "type Scalar = Int | String"
DESCRIBE = '''def describe(value: Scalar): String =
    value match
        case 0 => "zero"
        case n: Int if n > 0 => "positive"
        case text: String => text
        case _ => "negative"'''
ag("alias_and_assignment", 'var value: Scalar = 3\nval a = describe(value)\nvalue = "hello"\nval b = describe(value)\nprint(a)\nprint("|")\nprint(b)\nprint("\\n")',
   "positive|hello\n", declarations=SCALAR, definitions=DESCRIBE)
ag("literal_pattern", 'val x = describe(0)\nprint(x)\nprint("\\n")', "zero\n", declarations=SCALAR, definitions=DESCRIBE)
ag("guard_falls_through", 'val x = describe(-2)\nprint(x)\nprint("\\n")', "negative\n", declarations=SCALAR, definitions=DESCRIBE)
ag("alias_of_union_alias", 'val x: Outer = "nested"\nval text = describe(x)\nprint(text)\nprint("\\n")', "nested\n",
   declarations="type Inner = Int | String\ntype Outer = Inner | Unit",
   definitions='def describe(v: Outer): String =\n    v match\n        case s: String => s\n        case _ => "other"')
ag("union_reordered", 'val x: String | Int = 4\nval n = extract(x)\nprint(n)\nprint("\\n")', "4\n",
   definitions="def extract(v: Int | String): Int =\n    v match\n        case n: Int => n\n        case _ => 0")
ag("union_duplicate_members", 'val x: Int | String | Int = 6\nval n = extract(x)\nprint(n)\nprint("\\n")', "6\n",
   definitions="def extract(v: Int | String): Int =\n    v match\n        case n: Int => n\n        case _ => 0")
ag("union_parameter_numeric_conversion", 'val n = extract(5)\nval text = toStr(n)\nprint(text)\nprint("\\n")', "5.00\n",
   definitions="def extract(v: Double | String): Double =\n    v match\n        case n: Double => n\n        case _ => 0.0")
ag("union_return_value", 'val v = choose(1)\nval text = describe(v)\nprint(text)\nprint("\\n")', "one\n",
   definitions='def choose(n: Int): Int | String =\n    if n == 0 then\n        return 0\n    else\n        return "one"\n\ndef describe(v: Int | String): String =\n    v match\n        case s: String => s\n        case _ => "number"')
ag("first_matching_branch", 'val text = select(5)\nprint(text)\nprint("\\n")', "first\n",
   definitions='def select(v: Int | String): String =\n    v match\n        case n: Int if n > 0 => "first"\n        case n: Int if n > 3 => "second"\n        case _ => "other"')
ag("unit_literal_pattern", 'val text = select(())\nprint(text)\nprint("\\n")', "unit\n",
   definitions='def select(v: Int | Unit): String =\n    v match\n        case () => "unit"\n        case _ => "number"')
ag("string_literal_pattern", 'val text = select("ok")\nprint(text)\nprint("\\n")', "matched\n",
   definitions='def select(v: Int | String): String =\n    v match\n        case "ok" => "matched"\n        case _ => "other"')
POINT = "case class Point(x: Int, y: Int)"
ag("class_attributes", 'val p = Point(3, 4)\nval n = p.x + p.y\nprint(n)\nprint("\\n")', "7\n", declarations=POINT)
ag("class_parameter", 'val p = Point(3, 4)\nval n = total(p)\nprint(n)\nprint("\\n")', "7\n",
   declarations=POINT, definitions="def total(p: Point): Int = p.x + p.y")
ag("class_constructor_pattern", 'val p = Point(3, 4)\nval n = total(p)\nprint(n)\nprint("\\n")', "7\n",
   declarations=POINT,
   definitions="def total(v: Point): Int =\n    v match\n        case Point(a, b) => a + b\n        case _ => 0")
ag("class_union", 'val p: Int | Point = Point(3, 4)\nval n = total(p)\nprint(n)\nprint("\\n")', "7\n",
   declarations=POINT,
   definitions="def total(v: Int | Point): Int =\n    v match\n        case Point(a, b) => a + b\n        case n: Int => n\n        case _ => 0")
ag("class_distinct_names", 'val p: Point | Other = Other(9)\nval n = total(p)\nprint(n)\nprint("\\n")', "9\n",
   declarations="case class Point(x: Int)\ncase class Other(x: Int)",
   definitions="def total(v: Point | Other): Int =\n    v match\n        case Point(a) => a + 100\n        case Other(a) => a\n        case _ => 0")
ag("class_trailing_comma", 'val p = Point(\n    3,\n    4,\n    )\nval n = p.x + p.y\nprint(n)\nprint("\\n")', "7\n",
   declarations="case class Point(\n    x: Int,\n    y: Int,\n    )", spec="§1.2, §5.5")
ag("pattern_binding_scope", 'val n = 99\nval value: Int | String = 3\nval result = value match\n    case n: Int => n\n    case _ => 0\nprint(result)\nprint("|")\nprint(n)\nprint("\\n")', "3|99\n")
ag("branch_numeric_conversion", 'val v: Int | String = 1\nval n: Double = v match\n    case x: Int => x\n    case _ => 2.5\nval text = toStr(n)\nprint(text)\nprint("\\n")', "1.00\n")
ag("boolean_patterns", 'val value: Boolean | Int = true\nval text = value match\n    case true => "yes"\n    case false => "no"\n    case _ => "number"\nprint(text)\nprint("\\n")', "yes\n")
CASES[-1]["requires"].append("BOOLTHEN")
ag("compound_guard", 'val value: Int | String = 4\nval text = value match\n    case n: Int if n > 0 && n < 10 => "small"\n    case _ => "other"\nprint(text)\nprint("\\n")', "small\n")
CASES[-1]["requires"].append("BOOLTHEN")
ag("non_exhaustive_match", 'val value: Int | String = "x"\nval n = value match\n    case x: Int => x\nprint(n)', compiler=10)
ag("incompatible_match_branches", 'val value: Int | String = 1\nval n = value match\n    case x: Int => x\n    case _ => "x"\nprint(n)', compiler=6)
ag("wrong_guard_type", 'val value: Int | String = 1\nval n = value match\n    case x: Int if 1 => x\n    case _ => 0\nprint(n)', compiler=6)
ag("pattern_variable_outside_scope", 'val value: Int | String = 1\nval n = value match\n    case x: Int => x\n    case _ => 0\nprint(x)', compiler=3)
ag("undefined_alias", 'val x: Missing = 1\nprint(x)', compiler=3)
ag("redefined_alias", "()", declarations="type Scalar = Int | String\ntype Scalar = Int | Unit", compiler=4)
ag("union_assignment_rejects_other_type", 'var value: Int | String = 1\nvalue = ()', compiler=6)
ag("union_argument_too_wide", 'val value: Int | String = 1\nconsume(value)',
   definitions="def consume(x: Int): Unit = ()", compiler=5)
ag("constructor_wrong_arity", "val p = Point(1)", declarations=POINT, compiler=5)
ag("constructor_wrong_argument_type", 'val p = Point("x", 2)', declarations=POINT, compiler=5)
ag("class_redefinition", "()", declarations="case class Point(x: Int)\ncase class Point(y: Int)", compiler=4)

MAYBE_INT = 'def show(v: Int | Unit): String =\n    v match\n        case n: Int => toStr(n)\n        case _ => "none"'
for name, data, expected in [
    ("read_int_maybe_valid", "42\n", "42\n"),
    ("read_int_maybe_zero", "0\n", "0\n"),
    ("read_int_maybe_invalid", "abc\n", "none\n"),
    ("read_int_maybe_eof", "", "none\n"),
]:
    ag(name, 'val value = readIntMaybe()\nval text = show(value)\nprint(text)\nprint("\\n")',
       expected, stdin=data, definitions=MAYBE_INT)
MAYBE_DOUBLE = 'def show(v: Double | Unit): String =\n    v match\n        case n: Double => toStr(n)\n        case _ => "none"'
for name, data, expected in [
    ("read_double_maybe_valid", "2.5\n", "2.50\n"),
    ("read_double_maybe_zero", "0.0\n", "0.00\n"),
    ("read_double_maybe_invalid", "abc\n", "none\n"),
    ("read_double_maybe_eof", "", "none\n"),
]:
    ag(name, 'val value = readDoubleMaybe()\nval text = show(value)\nprint(text)\nprint("\\n")',
       expected, stdin=data, definitions=MAYBE_DOUBLE)

LS = ("BOOLTHEN", "CONTROL", "LISTS")
def ls(name, body, expected="", **kwargs):
    add("lists", name, body, expected, requires=LS, spec="§5.8", **kwargs)

ls("construct_and_print", 'val xs = List(1, 2, 3)\nprint(xs)\nprint("\\n")', "List(1, 2, 3)\n")
ls("empty_expected_type", 'val xs: List[Int] = List()\nprint(xs)\nprint("\\n")', "List()\n")
ls("singleton", 'val xs = List(7)\nprint(xs)\nprint("\\n")', "List(7)\n")
ls("boolean_elements", 'val xs: List[Boolean] = List(true, false)\nprint(xs)\nprint("\\n")', "List(true, false)\n")
ls("string_elements", 'val xs = List("a", "b")\nprint(xs)\nprint("\\n")', "List(a, b)\n")
ls("double_elements", 'val xs: List[Double] = List(1.0, 2.5)\nprint(xs)\nprint("\\n")', "List(1.00, 2.50)\n")
ls("numeric_element_conversion", 'val xs: List[Double] = List(1, 2.5)\nprint(xs)\nprint("\\n")', "List(1.00, 2.50)\n")
ls("unit_element", 'val xs: List[Unit] = List(())\nprint(xs)\nprint("\\n")', "List(())\n")
ls("nested_lists", 'val a = List(1, 2)\nval b = List(3)\nval xs: List[List[Int]] = List(a, b)\nprint(xs)\nprint("\\n")',
   "List(List(1, 2), List(3))\n")
ls("nested_empty_context", 'val xs: List[List[Int]] = List(List(), List(1))\nprint(xs)\nprint("\\n")',
   "List(List(), List(1))\n")
CASES[-1]["requires"].append("FUNEXP")
ls("trailing_comma", 'val xs = List(\n    1,\n    2,\n    )\nprint(xs)\nprint("\\n")', "List(1, 2)\n")
ls("first_last_index", 'val xs = List(10, 20, 30)\nval a = xs(0)\nval b = xs(2)\nprint(a)\nprint("|")\nprint(b)\nprint("\\n")', "10|30\n")
ls("head_tail", 'val xs = List(1, 2, 3)\nval first = xs.head\nval rest = xs.tail\nprint(first)\nprint("|")\nprint(rest)\nprint("\\n")', "1|List(2, 3)\n")
ls("singleton_tail", 'val xs = List(1)\nval rest = xs.tail\nprint(rest)\nprint("\\n")', "List()\n")
for name, method, expected in [
    ("drop_zero", "drop(0)", "List(1, 2, 3)"),
    ("drop_one", "drop(1)", "List(2, 3)"),
    ("drop_all", "drop(3)", "List()"),
    ("drop_more", "drop(10)", "List()"),
    ("take_zero", "take(0)", "List()"),
    ("take_two", "take(2)", "List(1, 2)"),
    ("take_more", "take(10)", "List(1, 2, 3)"),
    ("slice_middle", "slice(1, 3)", "List(2, 3)"),
    ("slice_empty", "slice(1, 1)", "List()"),
]:
    ls(name, f'val xs = List(1, 2, 3)\nval result = xs.{method}\nprint(result)\nprint("\\n")', expected + "\n")
ls("methods_preserve_original", 'val xs = List(1, 2, 3)\nval ys = xs.drop(1)\nprint(xs)\nprint("|")\nprint(ys)\nprint("\\n")',
   "List(1, 2, 3)|List(2, 3)\n")
ls("rebinding_preserves_alias", 'var xs = List(1, 2, 3)\nval ys = xs\nxs = xs.tail\nprint(ys)\nprint("|")\nprint(xs)\nprint("\\n")',
   "List(1, 2, 3)|List(2, 3)\n")
ls("method_chain", 'val xs = List(1, 2, 3, 4)\nval ys = xs.drop(1).take(2)\nprint(ys)\nprint("\\n")', "List(2, 3)\n")
ls("index_after_method", 'val xs = List(10, 20, 30)\nval n = xs.drop(2)(0)\nprint(n)\nprint("\\n")', "30\n")
ls("index_after_constructor", 'val n = List(10, 20)(1)\nprint(n)\nprint("\\n")', "20\n")
ls("concat", 'val a = List(1)\nval b = List(2, 3)\nval xs = a ++ b\nprint(xs)\nprint("\\n")', "List(1, 2, 3)\n")
ls("concat_empty", 'val a: List[Int] = List()\nval b = List(2)\nval xs = a ++ b ++ a\nprint(xs)\nprint("\\n")', "List(2)\n")
ls("three_string_representations", 'val xs = List(1, 2)\nval a = xs.toString\nval b = toStr(xs)\nprint(a)\nprint("|")\nprint(b)\nprint("|")\nprint(xs)\nprint("\\n")',
   "List(1, 2)|List(1, 2)|List(1, 2)\n")
for name, call, expected in [
    ("range_default", "List.range(1, 5)", "List(1, 2, 3, 4)"),
    ("range_step", "List.range(1, 8, 3)", "List(1, 4, 7)"),
    ("range_descending", "List.range(5, 0, -2)", "List(5, 3, 1)"),
    ("range_empty", "List.range(2, 2)", "List()"),
]:
    ls(name, f'val xs = {call}\nprint(xs)\nprint("\\n")', expected + "\n")
ls("inclusive_range_to_list", 'val xs: List[Int] = (1 to 3).toList\nprint(xs)\nprint("\\n")', "List(1, 2, 3)\n")
ls("exclusive_range_to_list", 'val xs: List[Int] = (1 until 3).toList\nprint(xs)\nprint("\\n")', "List(1, 2)\n")
ls("for_list", 'val xs = List(2, 4, 6)\nfor x <- xs do\n    print(x)\nprint("\\n")', "246\n")
ls("for_list_guard", 'val xs = List(1, 2, 3, 4)\nfor x <- xs if x > 2 do\n    print(x)\nprint("\\n")', "34\n")
ls("for_empty_list", 'val xs: List[Int] = List()\nfor x <- xs do\n    print("BAD")\nprint("OK\\n")', "OK\n")
ls("typed_parameter_and_return", 'val xs = identity(List())\nprint(xs)\nprint("\\n")', "List()\n",
   definitions="def identity(xs: List[Int]): List[Int] = xs")
CASES[-1]["requires"].append("FUNEXP")
ls("if_expression_lists", 'val xs: List[Int] = if true then List(1) else List(2)\nprint(xs)\nprint("\\n")', "List(1)\n")
ls("empty_without_context", "val xs = List()", compiler=6)
ls("incompatible_elements", 'val xs = List(1, "x")', compiler=6)
ls("incompatible_declared_type", 'val xs: List[Int] = List("x")', compiler=6)
ls("incompatible_assignment", 'var xs: List[Int] = List(1)\nxs = List("x")', compiler=6)
ls("incompatible_concat", 'val a = List(1)\nval b = List("x")\nval c = a ++ b', compiler=6)
ls("concat_non_list", 'val a = List(1)\nval b = a ++ 2', compiler=6)
ls("index_wrong_argument_type", 'val xs = List(1)\nval x = xs("zero")', compiler=5)
ls("drop_wrong_argument_type", 'val xs = List(1)\nval x = xs.drop("one")', compiler=5)
ls("list_unknown_element_type", 'val xs: List[Missing] = List()', compiler=3)
ls("list_missing_element_type", 'val xs: List[] = List()', compiler=2)
ls("list_extra_type_argument", 'val xs: List[Int, String] = List()', compiler=2)
ls("trailing_comma_same_line", 'val xs = List(1,)', compiler=2)
ls("list_loop_binding_immutable", 'val xs = List(1)\nfor x <- xs do\n    x = 2', compiler=7)

def runtime_case(name, body, expected="before\n", stdin="", status=40, requires=(), spec="§6"):
    add("runtime", name, body, expected, stdin=stdin, runtime=status, requires=requires, spec=spec)

runtime_case("integer_division_by_zero", 'val zero = readInt()\nprint("before\\n")\nval n = 1 / zero\nprint(n)',
             stdin="0\n", status=57, spec="§3, §6, §7.1")
for name, args in [
    ("substring_negative_start", '"abc", -1, 2'),
    ("substring_reversed_bounds", '"abc", 2, 1'),
    ("substring_end_too_large", '"abc", 0, 4'),
    ("substring_start_at_length", '"abc", 3, 3'),
    ("substring_empty_source", '"", 0, 0'),
]:
    runtime_case(name, f'print("before\\n")\nval s = subStr({args})\nprint(s)', spec="§4 subStr")
for name, body in [
    ("list_negative_index", 'val xs = List(1)\nval index = readInt()\nprint("before\\n")\nval x = xs(index)\nprint(x)'),
    ("list_index_at_length", 'val xs = List(1)\nval index = readInt()\nprint("before\\n")\nval x = xs(index)\nprint(x)'),
]:
    runtime_case(name, body, stdin="-1\n" if "negative" in name else "1\n", requires=LS, spec="§5.8")
runtime_case("list_empty_index", 'val xs: List[Int] = List()\nprint("before\\n")\nval x = xs(0)\nprint(x)', requires=LS, spec="§5.8")
runtime_case("list_empty_head", 'val xs: List[Int] = List()\nprint("before\\n")\nval x = xs.head\nprint(x)', requires=LS, spec="§5.8")
runtime_case("list_empty_tail", 'val xs: List[Int] = List()\nprint("before\\n")\nval x = xs.tail\nprint(x)', requires=LS, spec="§5.8")
runtime_case("list_range_zero_step", 'print("before\\n")\nval xs = List.range(1, 3, 0)\nprint(xs)', requires=LS, spec="§5.8")

def invalid(name, body=None, status=2, **kwargs):
    add("invalid", name, body, compiler=status, spec=kwargs.pop("spec", "§1–3, §6"), **kwargs)

for name, body in [
    ("unknown_character", "val x = #"),
    ("backtick_identifier", "val " + chr(96) + "x" + chr(96) + " = 1"),
    ("unclosed_string", 'val x = "abc'),
    ("actual_lf_in_string", 'val x = "a\nb"'),
    ("unsupported_escape", r'val x = "\q"'),
    ("unsupported_nul_escape", r'val x = "\0"'),
    ("unclosed_raw_string", 'val x = """abc'),
    ("unclosed_block_comment", "/* outer"),
    ("unclosed_nested_comment", "/* outer /* inner */"),
    ("integer_trailing_separator", "val x = 12_"),
    ("fraction_trailing_separator", "val x = 1.2_"),
    ("exponent_trailing_separator", "val x = 1e2_"),
    ("fraction_missing_digit", "val x = 1."),
    ("exponent_missing_digit", "val x = 1e"),
    ("exponent_sign_missing_digit", "val x = 1e+"),
    ("separator_after_decimal_point", "val x = 1._2"),
    ("separator_before_decimal_point", "val x = 1_.2"),
    ("separator_after_exponent_marker", "val x = 1e_2"),
    ("leading_zero_nonzero_int", "val x = 01"),
    ("lone_ampersand", "val x = 1 & 2"),
    ("code_after_multiline_comment", '/* one\nend */ print("bad")'),
]:
    invalid(name, body, status=1, spec="§1.1–1.2")
invalid("leading_tab", source='import ifj26.*\n@main\ndef main(): Unit =\n\tprint("bad")\n', status=1)
invalid("dedent_unknown_level", source='import ifj26.*\n@main\ndef main(): Unit =\n    if 1 < 2 then\n        print(1)\n      print(2)\n', status=1)
invalid("continuation_equal_indent", source='import ifj26.*\n@main\ndef main(): Unit =\n    val x = (1 +\n    2)\n', status=1)
invalid("square_continuation_equal_indent", source='import ifj26.*\n@main\ndef main(): Unit =\n    val xs: List[\n    Int] = List()\n', status=1, requires=LS)
invalid("missing_prologue", source='@main\ndef main(): Unit = ()\n')
invalid("wrong_prologue", source='import other.*\n@main\ndef main(): Unit = ()\n')
invalid("duplicate_prologue", source='import ifj26.*\nimport ifj26.*\n@main\ndef main(): Unit = ()\n')
invalid("missing_main", source='import ifj26.*\ndef helper(): Int = 1\n', status=3)
invalid("empty_block", source='import ifj26.*\n@main\ndef main(): Unit =\n')
invalid("keyword_variable_name", "val while = 1")
invalid("underscore_variable_name", "val _ = 1")
invalid("missing_initializer", "val x: Int")
invalid("missing_assignment_operator", "val x 1")
invalid("adjacent_number_identifier", "val x = 10abc", description="Maximal munch yields a number and identifier; syntax error, not one invalid identifier")
invalid("adjacent_literals", "val x = 1 2")
invalid("fraction_in_exponent", "val x = 2e0.01", description="2e0.01 is not one Double; adjacent literal tokens are syntactically invalid")
invalid("missing_right_parenthesis", "val x = (1 + 2")
invalid("extra_right_parenthesis", "val x = 1)")
invalid("missing_then", 'if 1 < 2\n    print("bad")')
invalid("missing_do", "while 1 < 2\n    ()")
invalid("relational_chain", "val x = 1 < 2 < 3", requires=BT)
invalid("unexpected_indent", source='import ifj26.*\n@main\ndef main(): Unit =\n    val x = 1\n      print(x)\n')
invalid("same_line_trailing_argument_comma", "val x = echo(1,)", definitions="def echo(x: Int): Int = x")
invalid("undefined_variable", "print(missing)", status=3)
invalid("undefined_function", "missing()", status=3)
invalid("variable_before_declaration", "print(x)\nval x = 1", status=3)
invalid("branch_variable_out_of_scope", 'if 1 < 2 then\n    val x = 1\nelse\n    val x = 2\nprint(x)', status=3)
invalid("variable_redefinition", "val x = 1\nval x = 2", status=4)
invalid("function_redefinition", "()", definitions="def f(): Int = 1\ndef f(): Int = 2", status=4)
invalid("duplicate_parameter", "()", definitions="def f(x: Int, x: Int): Int = x", status=4)
invalid("multiple_entry_functions", source='import ifj26.*\n@main\ndef first(): Unit = ()\n@main\ndef second(): Unit = ()\n', status=4)
invalid("entry_has_parameter", source='import ifj26.*\n@main\ndef main(x: Int): Unit = ()\n', status=5)
invalid("entry_wrong_return_type", source='import ifj26.*\n@main\ndef main(): Int = 1\n', status=6)
invalid("call_wrong_arity", "val x = echo()", definitions="def echo(x: Int): Int = x", status=5)
invalid("call_wrong_type", 'val x = echo("x")', definitions="def echo(x: Int): Int = x", status=5)
invalid("builtin_wrong_arity", 'val x = strLength("a", "b")', status=5)
invalid("builtin_wrong_type", "val x = strLength(1)", status=5)
invalid("to_int_wrong_type", 'val x = toInt("1")', status=5)
invalid("assignment_type_mismatch", 'var x = 1\nx = "a"', status=6)
invalid("initializer_type_mismatch", 'val x: String = 1', status=6)
invalid("double_to_int_assignment", 'val x: Int = 1.5', status=6)
invalid("arithmetic_wrong_type", 'val x = "a" - "b"', status=6)
invalid("string_implicit_concat", 'val x = "a" + 1', status=6)
invalid("unit_arithmetic", 'val x = () + 1', status=6)
invalid("wrong_condition_type", 'if 1 then\n    ()\nelse\n    ()', status=6)
invalid("immutable_variable_assignment", "val x = 1\nx = 2", status=7)
invalid("parameter_assignment", "()", definitions="def f(x: Int): Unit =\n    x = 2", status=7)
invalid("return_type_mismatch", "()", definitions='def f(): Int =\n    return "x"', status=6)
invalid("return_requires_explicit_type", "()", definitions="def f() =\n    return 1", status=6)
invalid("recursion_requires_explicit_type", "()", definitions="def f(n: Int) =\n    return f(n)", status=6)
invalid("unreachable_code_type_checked", "()", definitions='def f(): Int =\n    return 1\n    val x: String = 2', status=6)
invalid("unreachable_branch_type_checked", 'if 1 > 2 then\n    val x: String = 2\nelse\n    ()', status=6)
invalid("logical_not_int", "val x = !1", status=6, requires=BT)
invalid("logical_and_wrong_type", "val x = true && 1", status=6, requires=BT)
invalid("boolean_arithmetic", "val x = true + false", status=6, requires=BT)
invalid("short_circuit_undefined_still_checked", "val x = false && missing", status=3, requires=BT)

ALL = ("FUNEXP", "BOOLTHEN", "CONTROL", "ALGEBDT", "LISTS")
def combined(name, body, expected, **kwargs):
    add("combined", name, body, expected, requires=kwargs.pop("requires", ALL),
        spec=kwargs.pop("spec", "§5: meaningful combinations of implemented extensions"), **kwargs)

combined("showcase", '''val n = 2 * fact(5)
print(n)
print("\\n")
val flag = true || dangerous()
print(flag)
print("\\n")
val xs: List[Int] = List.range(1, 6)
for item <- xs if item > 2 do
    print(item)
print("\\n")
val selected = xs.drop(2)(0)
print(selected)
print("\\n")
val text = describe(Point(2, 3))
print(text)
print("\\n")
print(xs)
print("\\n")''', "240\ntrue\n345\n3\npoint 2:3\nList(1, 2, 3, 4, 5)\n",
    declarations="case class Point(x: Int, y: Int)\ntype Value = Int | String | Point",
    definitions='''def fact(n: Int): Int =
    if n <= 1 then
        return 1
    else
        return n * fact(n - 1)

def dangerous(): Boolean =
    print("BAD")
    true

def describe(v: Value): String =
    v match
        case p: Point => "point " + toStr(p.x) + ":" + toStr(p.y)
        case n: Int => toStr(n)
        case s: String => s
        case _ => "other"''')
combined("match_with_if_expression", 'val value: Int | String = 4\nval text = value match\n    case n: Int =>\n        if n > 0 then\n            "positive"\n        else\n            "nonpositive"\n    case _ => "text"\nprint(text)\nprint("\\n")', "positive\n",
         requires=("ALGEBDT", "BOOLTHEN", "CONTROL"))
combined("match_guard_and_if_expression", 'val value: Int | String = 12\nval text = value match\n    case n: Int if n > 0 && n < 20 =>\n        if n < 10 then\n            "small"\n        else\n            "large"\n    case _ => "other"\nprint(text)\nprint("\\n")', "large\n",
         requires=("ALGEBDT", "BOOLTHEN", "CONTROL"))
combined("guard_only_after_matching_pattern", 'val value: Int | String = "text"\nval text = value match\n    case n: Int if check(n) => "number"\n    case s: String => s\n    case _ => "other"\nprint(text)\nprint("\\n")', "text\n",
         definitions='def check(n: Int): Boolean =\n    print("BAD")\n    true',
         requires=("ALGEBDT", "BOOLTHEN", "FUNEXP"))
combined("list_union_elements", 'val xs: List[Int | String | Unit] = List(1, "x", ())\nprint(xs)\nprint("\\n")',
         "List(1, x, ())\n")
combined("list_of_classes", 'val a = Point(2, 3)\nval b = Point(4, 5)\nval xs: List[Point] = List(a, b)\nval p = xs(1)\nval n = p.x + p.y\nprint(n)\nprint("\\n")', "9\n",
         declarations=POINT)
combined("union_contains_list", 'val value: List[Int] | String = List(1, 2)\nprint(value)\nprint("\\n")', "List(1, 2)\n")
combined("recursive_list_processing", 'val xs = List(1, 2, 3)\nprint(total(xs, 3))\nprint("\\n")', "6\n",
         definitions="def total(xs: List[Int], n: Int): Int =\n    if n == 0 then\n        return 0\n    else\n        return xs.head + total(xs.tail, n - 1)")
combined("concat_operand_order", 'val xs = mark(1) ++ mark(2)\nprint(xs)\nprint("\\n")', "12List(1, 2)\n",
         definitions="def mark(n: Int): List[Int] =\n    print(n)\n    List(n)")
combined("list_source_evaluated_once", 'for n <- values() do\n    print(n)\nprint("\\n")', "F123\n",
         definitions='def values(): List[Int] =\n    print("F")\n    List(1, 2, 3)')
combined("short_circuit_avoids_empty_head", 'val empty: List[Int] = List()\nval x = true || empty.head > 0\nprint(x)\nprint("\\n")', "true\n",
         requires=LS)
combined("unit_as_nested_argument", 'print(consume(()))\nprint("\\n")', "8\n",
         definitions="def consume(x: Unit): Int = 8", requires=FX)
combined("mixed_bracket_continuation", source=None,
         body='val xs: List[\n    List[Int]\n    ] = List(\n    List(1, 2),\n    List(3),\n    )\nprint(xs)\nprint("\\n")',
         expected="List(List(1, 2), List(3))\n")


def write_fixtures():
    manifest = []
    catalog = [
        "# Katalog testu",
        "",
        "AI-generated fixtures; review against the referenced specification before submission.",
        "",
        "Expected stdout below belongs to the interpreted program, not to the compiler.",
        "Escapes such as \\n, \\t and \\r in this table denote the corresponding bytes.",
        "",
        "| ID | Skupina | Preklad | Beh | Ocekavany stdout (Python repr) | Specifikace |",
        "|---|---|---:|---:|---|---|",
    ]
    for case in CASES:
        directory = HERE / "cases" / case["group"]
        directory.mkdir(parents=True, exist_ok=True)
        base = directory / case["id"]
        files = {
            "source": (".ifj", case["source_text"]),
            "stdin": (".stdin", case["stdin_text"]),
            "stdout": (".expected.stdout", case["stdout_text"]),
            "compiler_status_file": (".expected.compiler.status", str(case["compiler_status"]) + "\n"),
        }
        if case["compiler_status"] == 0:
            files["runtime_status_file"] = (".expected.runtime.status", str(case["runtime_status"]) + "\n")
        record = {key: value for key, value in case.items() if not key.endswith("_text")}
        for key, (suffix, text) in files.items():
            path = Path(str(base) + suffix)
            path.write_bytes(text.encode("ascii"))
            record[key] = path.relative_to(HERE).as_posix()
        manifest.append(record)
        output = repr(case["stdout_text"]).replace("|", "\\|")
        runtime = "-" if case["runtime_status"] is None else str(case["runtime_status"])
        catalog.append(f"| {case['id']} | {case['group']} | {case['compiler_status']} | {runtime} | {output} | {case['spec']} |")
    document = {
        "format": 1,
        "authorship": "AI-generated by OpenAI Codex; human review required",
        "oracle": "IFJ26 specification and supplied course clarifications; never current compiler output",
        "extensions": list(ALL),
        "cases": manifest,
    }
    (HERE / "manifest.json").write_text(json.dumps(document, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    (HERE / "CASES.md").write_text("\n".join(catalog) + "\n", encoding="utf-8")
    from collections import Counter
    counts = Counter(case["group"] for case in CASES)
    print(f"Generated {len(CASES)} cases: " + ", ".join(f"{group}={count}" for group, count in sorted(counts.items())))


if __name__ == "__main__":
    write_fixtures()
