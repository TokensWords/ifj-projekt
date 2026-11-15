# Katalog testu

AI-generated fixtures; review against the referenced specification before submission.

Expected stdout below belongs to the interpreted program, not to the compiler.
Escapes such as \n, \t and \r in this table denote the corresponding bytes.

| ID | Skupina | Preklad | Beh | Ocekavany stdout (Python repr) | Specifikace |
|---|---|---:|---:|---|---|
| base_hello | base | 0 | 0 | 'Hello, IFJ26!\n' | §1–4 |
| base_empty_main | base | 0 | 0 | '' | §1–4 |
| base_inline_main | base | 0 | 0 | 'inline\n' | §1–4 |
| base_unit_value | base | 0 | 0 | '()\n' | §1–4 |
| base_unit_parameter | base | 0 | 0 | '7\n' | §1–4 |
| base_unit_nested_parentheses | base | 0 | 0 | '()\n' | §1–4 |
| base_semicolon_separators | base | 0 | 0 | '3\n' | §1–4 |
| base_shadowing | base | 0 | 0 | '91\n' | §1–4 |
| base_while_sum | base | 0 | 0 | '15\n' | §1–4 |
| base_while_zero_iterations | base | 0 | 0 | 'OK\n' | §1–4 |
| base_arbitrary_indentation | base | 0 | 0 | 'yes\n' | §1–4 |
| base_blank_and_comment_lines | base | 0 | 0 | '4\n' | §1–4 |
| base_inline_tab | base | 0 | 0 | '3\n' | §1–4 |
| base_crlf | base | 0 | 0 | 'CRLF\n' | §1–4 |
| base_eof_without_newline | base | 0 | 0 | 'EOF\n' | §1–4 |
| base_line_comment_eof | base | 0 | 0 | 'done\n' | §1–4 |
| base_nested_comments | base | 0 | 0 | 'OK\n' | §1–4 |
| base_comment_as_separator | base | 0 | 0 | '5\n' | §1–4 |
| base_block_comment_newline | base | 0 | 0 | '2\n' | §1–4 |
| base_long_identifier | base | 0 | 0 | '17\n' | §1–4 |
| base_underscore_identifiers | base | 0 | 0 | '6\n' | §1–4 |
| base_contextual_words_are_identifiers | base | 0 | 0 | '6\n' | §1–4 |
| base_case_sensitive_identifiers | base | 0 | 0 | '3\n' | §1–4 |
| base_precedence | base | 0 | 0 | '14\n' | §1.1, §3; numeric-separator forum clarification |
| base_parentheses | base | 0 | 0 | '20\n' | §1.1, §3; numeric-separator forum clarification |
| base_left_associative_subtraction | base | 0 | 0 | '12\n' | §1.1, §3; numeric-separator forum clarification |
| base_left_associative_division | base | 0 | 0 | '5\n' | §1.1, §3; numeric-separator forum clarification |
| base_division_truncates_to_zero | base | 0 | 0 | '-3\n' | §1.1, §3; numeric-separator forum clarification |
| base_division_negative_divisor | base | 0 | 0 | '-3\n' | §1.1, §3; numeric-separator forum clarification |
| base_integer_separators | base | 0 | 0 | '1020\n' | §1.1, §3; numeric-separator forum clarification |
| base_negative_zero | base | 0 | 0 | '0\n' | §1.1, §3; numeric-separator forum clarification |
| base_unary_minus | base | 0 | 0 | '-20\n' | §1.1, §3; numeric-separator forum clarification |
| base_zero | base | 0 | 0 | '0\n' | §1.1, §3; numeric-separator forum clarification |
| base_double_fraction | base | 0 | 0 | '12.50\n' | §1.1, §1.3, §3–4 |
| base_double_without_integer_part | base | 0 | 0 | '0.50\n' | §1.1, §1.3, §3–4 |
| base_double_exponent | base | 0 | 0 | '2000.00\n' | §1.1, §1.3, §3–4 |
| base_double_negative_exponent | base | 0 | 0 | '0.20\n' | §1.1, §1.3, §3–4 |
| base_double_exponent_leading_zero | base | 0 | 0 | '20.00\n' | §1.1, §1.3, §3–4 |
| base_double_negative_exponent_leading_zero | base | 0 | 0 | '0.20\n' | §1.1, §1.3, §3–4 |
| base_double_repeated_separators | base | 0 | 0 | '12.50\n' | §1.1, §1.3, §3–4 |
| base_double_leading_zero | base | 0 | 0 | '0.25\n' | §1.1, §1.3, §3–4 |
| base_double_suffix | base | 0 | 0 | '4.00\n' | §1.1, §1.3, §3–4 |
| base_mixed_numeric_arithmetic | base | 0 | 0 | '3.50\n' | §1.1, §1.3, §3–4 |
| base_double_division | base | 0 | 0 | '3.50\n' | §1.1, §1.3, §3–4 |
| base_int_to_double_assignment | base | 0 | 0 | '7.00\n' | §1–4 |
| base_int_to_double_argument | base | 0 | 0 | '3.00\n' | §1–4 |
| base_int_to_double_return | base | 0 | 0 | '4.00\n' | §1–4 |
| base_to_int_truncation | base | 0 | 0 | '3,-3\n' | §1–4 |
| base_string_concat | base | 0 | 0 | 'abcd\n' | §1–4 |
| base_string_escapes | base | 0 | 0 | 'A\nB\t"\\\rZ' | §1–4 |
| base_string_empty | base | 0 | 0 | '0\n' | §1–4 |
| base_string_long | base | 0 | 0 | 'abababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababababab\n' | §1–4 |
| base_raw_string | base | 0 | 0 | 'A\nB\\nC\n' | §1–4 |
| base_raw_quotes_and_comment_text | base | 0 | 0 | 'one " two "" /* data */ // data\n' | §1–4 |
| base_string_builtin_boundaries | base | 0 | 0 | '0\|bcd\|97\|-1\|-1\|-1\|A\n' | §1–4 |
| base_string_comparison | base | 0 | 0 | '111\n' | §1–4 |
| base_numeric_comparison | base | 0 | 0 | '111\n' | §1–4 |
| base_continuation_logical_baseline | base | 0 | 0 | '6\n' | §1–4 |
| base_trailing_comma_arguments | base | 0 | 0 | '5\n' | §1.2 |
| base_trailing_comma_parameters | base | 0 | 0 | '5\n' | §1.2 |
| base_call_before_definition | base | 0 | 0 | '8\n' | §1–4 |
| base_return_from_nested_block | base | 0 | 0 | '7\n' | §1–4 |
| base_explicit_unit_discards_value | base | 0 | 0 | 'side()\n' | §1–4 |
| base_function_result_last_expression | base | 0 | 0 | '5\n' | §1–4 |
| base_while_block_returns_unit | base | 0 | 0 | '()\n' | §1–4 |
| base_recursive_factorial | base | 0 | 0 | '120\n' | §1–4 |
| base_mutual_recursion | base | 0 | 0 | '1\n' | §1–4 |
| base_read_line | base | 0 | 0 | 'alpha\nbeta\n0\n' | §1–4 |
| base_read_line_without_final_lf | base | 0 | 0 | 'tail\|' | §1–4 |
| base_read_int_valid | base | 0 | 0 | '-42\n' | §4 |
| base_read_int_invalid | base | 0 | 0 | '0\n' | §4 |
| base_read_int_whitespace | base | 0 | 0 | '0\n' | §4 |
| base_read_int_underscore | base | 0 | 0 | '0\n' | §4 |
| base_read_int_eof | base | 0 | 0 | '0\n' | §4 |
| base_read_double_valid | base | 0 | 0 | '2.50\n' | §4 |
| base_read_double_invalid | base | 0 | 0 | '0.00\n' | §4 |
| base_read_double_whitespace | base | 0 | 0 | '0.00\n' | §4 |
| base_read_double_eof | base | 0 | 0 | '0.00\n' | §4 |
| funexp_nested_calls | funexp | 0 | 0 | '15\n' | §5.1 |
| funexp_calls_in_arithmetic | funexp | 0 | 0 | '22\n' | §5.1 |
| funexp_arbitrary_argument_expression | funexp | 0 | 0 | '16\n' | §5.1 |
| funexp_argument_order | funexp | 0 | 0 | '123\n' | §5.1 |
| funexp_operand_order | funexp | 0 | 0 | '123=7\n' | §5.1 |
| funexp_nested_argument_order | funexp | 0 | 0 | '123=6\n' | §5.1 |
| funexp_recursive_expression | funexp | 0 | 0 | '55\n' | §5.1 |
| funexp_builtins_nested | funexp | 0 | 0 | '3\n' | §5.1 |
| funexp_call_in_while_condition | funexp | 0 | 0 | '012\n' | §5.1 |
| funexp_call_in_return | funexp | 0 | 0 | '10\n' | §5.1 |
| funexp_frame_values_survive_calls | funexp | 0 | 0 | '15\n' | §5.1 |
| boolthen_true_literal | boolthen | 0 | 0 | 'true\n' | §5.2 |
| boolthen_false_literal | boolthen | 0 | 0 | 'false\n' | §5.2 |
| boolthen_negation | boolthen | 0 | 0 | 'true\n' | §5.2 |
| boolthen_double_negation | boolthen | 0 | 0 | 'true\n' | §5.2 |
| boolthen_logical_precedence | boolthen | 0 | 0 | 'true\n' | §5.2 |
| boolthen_logical_parentheses | boolthen | 0 | 0 | 'false\n' | §5.2 |
| boolthen_relational_precedence | boolthen | 0 | 0 | 'true\n' | §5.2 |
| boolthen_boolean_equality | boolthen | 0 | 0 | 'true\n' | §5.2 |
| boolthen_boolean_inequality | boolthen | 0 | 0 | 'true\n' | §5.2 |
| boolthen_and_00 | boolthen | 0 | 0 | 'false\n' | §5.2 |
| boolthen_and_01 | boolthen | 0 | 0 | 'false\n' | §5.2 |
| boolthen_and_10 | boolthen | 0 | 0 | 'false\n' | §5.2 |
| boolthen_and_11 | boolthen | 0 | 0 | 'true\n' | §5.2 |
| boolthen_or_00 | boolthen | 0 | 0 | 'false\n' | §5.2 |
| boolthen_or_01 | boolthen | 0 | 0 | 'true\n' | §5.2 |
| boolthen_or_10 | boolthen | 0 | 0 | 'true\n' | §5.2 |
| boolthen_or_11 | boolthen | 0 | 0 | 'true\n' | §5.2 |
| boolthen_boolean_parameter_return | boolthen | 0 | 0 | 'false\n' | §5.2 |
| boolthen_boolean_variable_assignment | boolthen | 0 | 0 | 'true\n' | §5.2 |
| boolthen_boolean_to_str | boolthen | 0 | 0 | 'false\n' | §5.2 |
| boolthen_if_without_else | boolthen | 0 | 0 | 'OK\n' | §5.2 |
| boolthen_else_if | boolthen | 0 | 0 | 'zero\n' | §5.2 |
| boolthen_boolean_while | boolthen | 0 | 0 | '3\n' | §5.2 |
| boolthen_read_boolean | boolthen | 0 | 0 | 'true,false\n' | §5.2 |
| boolthen_short_and_skips | boolthen | 0 | 0 | 'false\n' | §5.2 |
| boolthen_short_or_skips | boolthen | 0 | 0 | 'true\n' | §5.2 |
| boolthen_short_and_runs | boolthen | 0 | 0 | 'Etrue\n' | §5.2 |
| boolthen_short_or_runs | boolthen | 0 | 0 | 'Etrue\n' | §5.2 |
| boolthen_short_circuit_avoids_division | boolthen | 0 | 0 | 'false\n' | §5.2 |
| boolthen_short_circuit_still_type_checks | boolthen | 6 | - | '' | §5.2 |
| control_if_expression_true | control | 0 | 0 | '10\n' | §5.3 |
| control_if_expression_false | control | 0 | 0 | '20\n' | §5.3 |
| control_if_expression_nested_else | control | 0 | 0 | '2\n' | §5.3 |
| control_if_expression_mixed_numeric | control | 0 | 0 | '1.00\n' | §5.3 |
| control_if_expression_multiline | control | 0 | 0 | '20\n' | §5.3 |
| control_if_expression_mixed_layout | control | 0 | 0 | '20\n' | §5.3 |
| control_if_inline_statement | control | 0 | 0 | 'YES\n' | §5.3 |
| control_if_only_selected_branch | control | 0 | 0 | 'G7\n' | §5.3 |
| control_for_inclusive | control | 0 | 0 | '1234\n' | §5.3 |
| control_for_exclusive | control | 0 | 0 | '123\n' | §5.3 |
| control_for_step | control | 0 | 0 | '135\n' | §5.3 |
| control_for_descending | control | 0 | 0 | '531\n' | §5.3 |
| control_for_single_inclusive | control | 0 | 0 | '3\n' | §5.3 |
| control_for_empty_exclusive | control | 0 | 0 | '\n' | §5.3 |
| control_for_wrong_direction_empty | control | 0 | 0 | '\n' | §5.3 |
| control_for_negative_bounds | control | 0 | 0 | '-2-101\n' | §5.3 |
| control_for_guard | control | 0 | 0 | '345\n' | §5.3 |
| control_for_two_guards | control | 0 | 0 | '234\n' | §5.3 |
| control_for_inline_body | control | 0 | 0 | '123\n' | §5.3 |
| control_for_multiline_header | control | 0 | 0 | '013\n' | §5.3 |
| control_for_shadowing | control | 0 | 0 | '1299\n' | §5.3 |
| control_for_nested | control | 0 | 0 | '11122122\n' | §5.3 |
| control_for_bounds_evaluated_once | control | 0 | 0 | '131123\n' | §5.3 |
| control_for_guard_order | control | 0 | 0 | 'G2B2G3B3\n' | §5.3 |
| control_return_from_for | control | 0 | 0 | '3\n' | §5.3 |
| control_if_expression_missing_else | control | 2 | - | '' | §5.3 |
| control_if_expression_incompatible_branches | control | 6 | - | '' | §5.3 |
| control_for_bound_type | control | 6 | - | '' | §5.3 |
| control_for_step_type | control | 6 | - | '' | §5.3 |
| control_for_guard_type | control | 6 | - | '' | §5.3 |
| control_for_immutable_binding | control | 7 | - | '' | §5.3 |
| control_for_binding_outside_scope | control | 3 | - | '' | §5.3 |
| control_for_literal_zero_step | control | 10 | - | '' | §5.3 |
| control_for_runtime_zero_step | control | 0 | 40 | 'before\n' | §5.3 |
| algebdt_alias_and_assignment | algebdt | 0 | 0 | 'positive\|hello\n' | §5.5 |
| algebdt_literal_pattern | algebdt | 0 | 0 | 'zero\n' | §5.5 |
| algebdt_guard_falls_through | algebdt | 0 | 0 | 'negative\n' | §5.5 |
| algebdt_alias_of_union_alias | algebdt | 0 | 0 | 'nested\n' | §5.5 |
| algebdt_union_reordered | algebdt | 0 | 0 | '4\n' | §5.5 |
| algebdt_union_duplicate_members | algebdt | 0 | 0 | '6\n' | §5.5 |
| algebdt_union_parameter_numeric_conversion | algebdt | 0 | 0 | '5.00\n' | §5.5 |
| algebdt_union_return_value | algebdt | 0 | 0 | 'one\n' | §5.5 |
| algebdt_first_matching_branch | algebdt | 0 | 0 | 'first\n' | §5.5 |
| algebdt_unit_literal_pattern | algebdt | 0 | 0 | 'unit\n' | §5.5 |
| algebdt_string_literal_pattern | algebdt | 0 | 0 | 'matched\n' | §5.5 |
| algebdt_class_attributes | algebdt | 0 | 0 | '7\n' | §5.5 |
| algebdt_class_parameter | algebdt | 0 | 0 | '7\n' | §5.5 |
| algebdt_class_constructor_pattern | algebdt | 0 | 0 | '7\n' | §5.5 |
| algebdt_class_union | algebdt | 0 | 0 | '7\n' | §5.5 |
| algebdt_class_distinct_names | algebdt | 0 | 0 | '9\n' | §5.5 |
| algebdt_class_trailing_comma | algebdt | 0 | 0 | '7\n' | §1.2, §5.5 |
| algebdt_pattern_binding_scope | algebdt | 0 | 0 | '3\|99\n' | §5.5 |
| algebdt_branch_numeric_conversion | algebdt | 0 | 0 | '1.00\n' | §5.5 |
| algebdt_boolean_patterns | algebdt | 0 | 0 | 'yes\n' | §5.5 |
| algebdt_compound_guard | algebdt | 0 | 0 | 'small\n' | §5.5 |
| algebdt_non_exhaustive_match | algebdt | 10 | - | '' | §5.5 |
| algebdt_incompatible_match_branches | algebdt | 6 | - | '' | §5.5 |
| algebdt_wrong_guard_type | algebdt | 6 | - | '' | §5.5 |
| algebdt_pattern_variable_outside_scope | algebdt | 3 | - | '' | §5.5 |
| algebdt_undefined_alias | algebdt | 3 | - | '' | §5.5 |
| algebdt_redefined_alias | algebdt | 4 | - | '' | §5.5 |
| algebdt_union_assignment_rejects_other_type | algebdt | 6 | - | '' | §5.5 |
| algebdt_union_argument_too_wide | algebdt | 5 | - | '' | §5.5 |
| algebdt_constructor_wrong_arity | algebdt | 5 | - | '' | §5.5 |
| algebdt_constructor_wrong_argument_type | algebdt | 5 | - | '' | §5.5 |
| algebdt_class_redefinition | algebdt | 4 | - | '' | §5.5 |
| algebdt_read_int_maybe_valid | algebdt | 0 | 0 | '42\n' | §5.5 |
| algebdt_read_int_maybe_zero | algebdt | 0 | 0 | '0\n' | §5.5 |
| algebdt_read_int_maybe_invalid | algebdt | 0 | 0 | 'none\n' | §5.5 |
| algebdt_read_int_maybe_eof | algebdt | 0 | 0 | 'none\n' | §5.5 |
| algebdt_read_double_maybe_valid | algebdt | 0 | 0 | '2.50\n' | §5.5 |
| algebdt_read_double_maybe_zero | algebdt | 0 | 0 | '0.00\n' | §5.5 |
| algebdt_read_double_maybe_invalid | algebdt | 0 | 0 | 'none\n' | §5.5 |
| algebdt_read_double_maybe_eof | algebdt | 0 | 0 | 'none\n' | §5.5 |
| lists_construct_and_print | lists | 0 | 0 | 'List(1, 2, 3)\n' | §5.8 |
| lists_empty_expected_type | lists | 0 | 0 | 'List()\n' | §5.8 |
| lists_singleton | lists | 0 | 0 | 'List(7)\n' | §5.8 |
| lists_boolean_elements | lists | 0 | 0 | 'List(true, false)\n' | §5.8 |
| lists_string_elements | lists | 0 | 0 | 'List(a, b)\n' | §5.8 |
| lists_double_elements | lists | 0 | 0 | 'List(1.00, 2.50)\n' | §5.8 |
| lists_numeric_element_conversion | lists | 0 | 0 | 'List(1.00, 2.50)\n' | §5.8 |
| lists_unit_element | lists | 0 | 0 | 'List(())\n' | §5.8 |
| lists_nested_lists | lists | 0 | 0 | 'List(List(1, 2), List(3))\n' | §5.8 |
| lists_nested_empty_context | lists | 0 | 0 | 'List(List(), List(1))\n' | §5.8 |
| lists_trailing_comma | lists | 0 | 0 | 'List(1, 2)\n' | §5.8 |
| lists_first_last_index | lists | 0 | 0 | '10\|30\n' | §5.8 |
| lists_head_tail | lists | 0 | 0 | '1\|List(2, 3)\n' | §5.8 |
| lists_singleton_tail | lists | 0 | 0 | 'List()\n' | §5.8 |
| lists_drop_zero | lists | 0 | 0 | 'List(1, 2, 3)\n' | §5.8 |
| lists_drop_one | lists | 0 | 0 | 'List(2, 3)\n' | §5.8 |
| lists_drop_all | lists | 0 | 0 | 'List()\n' | §5.8 |
| lists_drop_more | lists | 0 | 0 | 'List()\n' | §5.8 |
| lists_take_zero | lists | 0 | 0 | 'List()\n' | §5.8 |
| lists_take_two | lists | 0 | 0 | 'List(1, 2)\n' | §5.8 |
| lists_take_more | lists | 0 | 0 | 'List(1, 2, 3)\n' | §5.8 |
| lists_slice_middle | lists | 0 | 0 | 'List(2, 3)\n' | §5.8 |
| lists_slice_empty | lists | 0 | 0 | 'List()\n' | §5.8 |
| lists_methods_preserve_original | lists | 0 | 0 | 'List(1, 2, 3)\|List(2, 3)\n' | §5.8 |
| lists_rebinding_preserves_alias | lists | 0 | 0 | 'List(1, 2, 3)\|List(2, 3)\n' | §5.8 |
| lists_method_chain | lists | 0 | 0 | 'List(2, 3)\n' | §5.8 |
| lists_index_after_method | lists | 0 | 0 | '30\n' | §5.8 |
| lists_index_after_constructor | lists | 0 | 0 | '20\n' | §5.8 |
| lists_concat | lists | 0 | 0 | 'List(1, 2, 3)\n' | §5.8 |
| lists_concat_empty | lists | 0 | 0 | 'List(2)\n' | §5.8 |
| lists_three_string_representations | lists | 0 | 0 | 'List(1, 2)\|List(1, 2)\|List(1, 2)\n' | §5.8 |
| lists_range_default | lists | 0 | 0 | 'List(1, 2, 3, 4)\n' | §5.8 |
| lists_range_step | lists | 0 | 0 | 'List(1, 4, 7)\n' | §5.8 |
| lists_range_descending | lists | 0 | 0 | 'List(5, 3, 1)\n' | §5.8 |
| lists_range_empty | lists | 0 | 0 | 'List()\n' | §5.8 |
| lists_inclusive_range_to_list | lists | 0 | 0 | 'List(1, 2, 3)\n' | §5.8 |
| lists_exclusive_range_to_list | lists | 0 | 0 | 'List(1, 2)\n' | §5.8 |
| lists_for_list | lists | 0 | 0 | '246\n' | §5.8 |
| lists_for_list_guard | lists | 0 | 0 | '34\n' | §5.8 |
| lists_for_empty_list | lists | 0 | 0 | 'OK\n' | §5.8 |
| lists_typed_parameter_and_return | lists | 0 | 0 | 'List()\n' | §5.8 |
| lists_if_expression_lists | lists | 0 | 0 | 'List(1)\n' | §5.8 |
| lists_empty_without_context | lists | 6 | - | '' | §5.8 |
| lists_incompatible_elements | lists | 6 | - | '' | §5.8 |
| lists_incompatible_declared_type | lists | 6 | - | '' | §5.8 |
| lists_incompatible_assignment | lists | 6 | - | '' | §5.8 |
| lists_incompatible_concat | lists | 6 | - | '' | §5.8 |
| lists_concat_non_list | lists | 6 | - | '' | §5.8 |
| lists_index_wrong_argument_type | lists | 5 | - | '' | §5.8 |
| lists_drop_wrong_argument_type | lists | 5 | - | '' | §5.8 |
| lists_list_unknown_element_type | lists | 3 | - | '' | §5.8 |
| lists_list_missing_element_type | lists | 2 | - | '' | §5.8 |
| lists_list_extra_type_argument | lists | 2 | - | '' | §5.8 |
| lists_trailing_comma_same_line | lists | 2 | - | '' | §5.8 |
| lists_list_loop_binding_immutable | lists | 7 | - | '' | §5.8 |
| runtime_integer_division_by_zero | runtime | 0 | 57 | 'before\n' | §3, §6, §7.1 |
| runtime_substring_negative_start | runtime | 0 | 40 | 'before\n' | §4 subStr |
| runtime_substring_reversed_bounds | runtime | 0 | 40 | 'before\n' | §4 subStr |
| runtime_substring_end_too_large | runtime | 0 | 40 | 'before\n' | §4 subStr |
| runtime_substring_start_at_length | runtime | 0 | 40 | 'before\n' | §4 subStr |
| runtime_substring_empty_source | runtime | 0 | 40 | 'before\n' | §4 subStr |
| runtime_list_negative_index | runtime | 0 | 40 | 'before\n' | §5.8 |
| runtime_list_index_at_length | runtime | 0 | 40 | 'before\n' | §5.8 |
| runtime_list_empty_index | runtime | 0 | 40 | 'before\n' | §5.8 |
| runtime_list_empty_head | runtime | 0 | 40 | 'before\n' | §5.8 |
| runtime_list_empty_tail | runtime | 0 | 40 | 'before\n' | §5.8 |
| runtime_list_range_zero_step | runtime | 0 | 40 | 'before\n' | §5.8 |
| invalid_unknown_character | invalid | 1 | - | '' | §1.1–1.2 |
| invalid_backtick_identifier | invalid | 1 | - | '' | §1.1–1.2 |
| invalid_unclosed_string | invalid | 1 | - | '' | §1.1–1.2 |
| invalid_actual_lf_in_string | invalid | 1 | - | '' | §1.1–1.2 |
| invalid_unsupported_escape | invalid | 1 | - | '' | §1.1–1.2 |
| invalid_unsupported_nul_escape | invalid | 1 | - | '' | §1.1–1.2 |
| invalid_unclosed_raw_string | invalid | 1 | - | '' | §1.1–1.2 |
| invalid_unclosed_block_comment | invalid | 1 | - | '' | §1.1–1.2 |
| invalid_unclosed_nested_comment | invalid | 1 | - | '' | §1.1–1.2 |
| invalid_integer_trailing_separator | invalid | 1 | - | '' | §1.1–1.2 |
| invalid_fraction_trailing_separator | invalid | 1 | - | '' | §1.1–1.2 |
| invalid_exponent_trailing_separator | invalid | 1 | - | '' | §1.1–1.2 |
| invalid_fraction_missing_digit | invalid | 1 | - | '' | §1.1–1.2 |
| invalid_exponent_missing_digit | invalid | 1 | - | '' | §1.1–1.2 |
| invalid_exponent_sign_missing_digit | invalid | 1 | - | '' | §1.1–1.2 |
| invalid_separator_after_decimal_point | invalid | 1 | - | '' | §1.1–1.2 |
| invalid_separator_before_decimal_point | invalid | 1 | - | '' | §1.1–1.2 |
| invalid_separator_after_exponent_marker | invalid | 1 | - | '' | §1.1–1.2 |
| invalid_leading_zero_nonzero_int | invalid | 1 | - | '' | §1.1–1.2 |
| invalid_lone_ampersand | invalid | 1 | - | '' | §1.1–1.2 |
| invalid_code_after_multiline_comment | invalid | 1 | - | '' | §1.1–1.2 |
| invalid_leading_tab | invalid | 1 | - | '' | §1–3, §6 |
| invalid_dedent_unknown_level | invalid | 1 | - | '' | §1–3, §6 |
| invalid_continuation_equal_indent | invalid | 1 | - | '' | §1–3, §6 |
| invalid_square_continuation_equal_indent | invalid | 1 | - | '' | §1–3, §6 |
| invalid_missing_prologue | invalid | 2 | - | '' | §1–3, §6 |
| invalid_wrong_prologue | invalid | 2 | - | '' | §1–3, §6 |
| invalid_duplicate_prologue | invalid | 2 | - | '' | §1–3, §6 |
| invalid_missing_main | invalid | 3 | - | '' | §1–3, §6 |
| invalid_empty_block | invalid | 2 | - | '' | §1–3, §6 |
| invalid_keyword_variable_name | invalid | 2 | - | '' | §1–3, §6 |
| invalid_underscore_variable_name | invalid | 2 | - | '' | §1–3, §6 |
| invalid_missing_initializer | invalid | 2 | - | '' | §1–3, §6 |
| invalid_missing_assignment_operator | invalid | 2 | - | '' | §1–3, §6 |
| invalid_adjacent_number_identifier | invalid | 2 | - | '' | §1–3, §6 |
| invalid_adjacent_literals | invalid | 2 | - | '' | §1–3, §6 |
| invalid_fraction_in_exponent | invalid | 2 | - | '' | §1–3, §6 |
| invalid_missing_right_parenthesis | invalid | 2 | - | '' | §1–3, §6 |
| invalid_extra_right_parenthesis | invalid | 2 | - | '' | §1–3, §6 |
| invalid_missing_then | invalid | 2 | - | '' | §1–3, §6 |
| invalid_missing_do | invalid | 2 | - | '' | §1–3, §6 |
| invalid_relational_chain | invalid | 2 | - | '' | §1–3, §6 |
| invalid_unexpected_indent | invalid | 2 | - | '' | §1–3, §6 |
| invalid_same_line_trailing_argument_comma | invalid | 2 | - | '' | §1–3, §6 |
| invalid_undefined_variable | invalid | 3 | - | '' | §1–3, §6 |
| invalid_undefined_function | invalid | 3 | - | '' | §1–3, §6 |
| invalid_variable_before_declaration | invalid | 3 | - | '' | §1–3, §6 |
| invalid_branch_variable_out_of_scope | invalid | 3 | - | '' | §1–3, §6 |
| invalid_variable_redefinition | invalid | 4 | - | '' | §1–3, §6 |
| invalid_function_redefinition | invalid | 4 | - | '' | §1–3, §6 |
| invalid_duplicate_parameter | invalid | 4 | - | '' | §1–3, §6 |
| invalid_multiple_entry_functions | invalid | 4 | - | '' | §1–3, §6 |
| invalid_entry_has_parameter | invalid | 5 | - | '' | §1–3, §6 |
| invalid_entry_wrong_return_type | invalid | 6 | - | '' | §1–3, §6 |
| invalid_call_wrong_arity | invalid | 5 | - | '' | §1–3, §6 |
| invalid_call_wrong_type | invalid | 5 | - | '' | §1–3, §6 |
| invalid_builtin_wrong_arity | invalid | 5 | - | '' | §1–3, §6 |
| invalid_builtin_wrong_type | invalid | 5 | - | '' | §1–3, §6 |
| invalid_to_int_wrong_type | invalid | 5 | - | '' | §1–3, §6 |
| invalid_assignment_type_mismatch | invalid | 6 | - | '' | §1–3, §6 |
| invalid_initializer_type_mismatch | invalid | 6 | - | '' | §1–3, §6 |
| invalid_double_to_int_assignment | invalid | 6 | - | '' | §1–3, §6 |
| invalid_arithmetic_wrong_type | invalid | 6 | - | '' | §1–3, §6 |
| invalid_string_implicit_concat | invalid | 6 | - | '' | §1–3, §6 |
| invalid_unit_arithmetic | invalid | 6 | - | '' | §1–3, §6 |
| invalid_wrong_condition_type | invalid | 6 | - | '' | §1–3, §6 |
| invalid_immutable_variable_assignment | invalid | 7 | - | '' | §1–3, §6 |
| invalid_parameter_assignment | invalid | 7 | - | '' | §1–3, §6 |
| invalid_return_type_mismatch | invalid | 6 | - | '' | §1–3, §6 |
| invalid_return_requires_explicit_type | invalid | 6 | - | '' | §1–3, §6 |
| invalid_recursion_requires_explicit_type | invalid | 6 | - | '' | §1–3, §6 |
| invalid_unreachable_code_type_checked | invalid | 6 | - | '' | §1–3, §6 |
| invalid_unreachable_branch_type_checked | invalid | 6 | - | '' | §1–3, §6 |
| invalid_logical_not_int | invalid | 6 | - | '' | §1–3, §6 |
| invalid_logical_and_wrong_type | invalid | 6 | - | '' | §1–3, §6 |
| invalid_boolean_arithmetic | invalid | 6 | - | '' | §1–3, §6 |
| invalid_short_circuit_undefined_still_checked | invalid | 3 | - | '' | §1–3, §6 |
| combined_showcase | combined | 0 | 0 | '240\ntrue\n345\n3\npoint 2:3\nList(1, 2, 3, 4, 5)\n' | §5: meaningful combinations of implemented extensions |
| combined_match_with_if_expression | combined | 0 | 0 | 'positive\n' | §5: meaningful combinations of implemented extensions |
| combined_match_guard_and_if_expression | combined | 0 | 0 | 'large\n' | §5: meaningful combinations of implemented extensions |
| combined_guard_only_after_matching_pattern | combined | 0 | 0 | 'text\n' | §5: meaningful combinations of implemented extensions |
| combined_list_union_elements | combined | 0 | 0 | 'List(1, x, ())\n' | §5: meaningful combinations of implemented extensions |
| combined_list_of_classes | combined | 0 | 0 | '9\n' | §5: meaningful combinations of implemented extensions |
| combined_union_contains_list | combined | 0 | 0 | 'List(1, 2)\n' | §5: meaningful combinations of implemented extensions |
| combined_recursive_list_processing | combined | 0 | 0 | '6\n' | §5: meaningful combinations of implemented extensions |
| combined_concat_operand_order | combined | 0 | 0 | '12List(1, 2)\n' | §5: meaningful combinations of implemented extensions |
| combined_list_source_evaluated_once | combined | 0 | 0 | 'F123\n' | §5: meaningful combinations of implemented extensions |
| combined_short_circuit_avoids_empty_head | combined | 0 | 0 | 'true\n' | §5: meaningful combinations of implemented extensions |
| combined_unit_as_nested_argument | combined | 0 | 0 | '8\n' | §5: meaningful combinations of implemented extensions |
| combined_mixed_bracket_continuation | combined | 0 | 0 | 'List(List(1, 2), List(3))\n' | §5: meaningful combinations of implemented extensions |
