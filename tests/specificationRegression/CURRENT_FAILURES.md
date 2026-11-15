# Zaznamenany beh 2026-11-15

Zdrojovy commit prekladace: 9ae24ba6b2b8a6dd936a42b04c893f0f61664509.

Pouzit skutecny prekladac sestaveny GCC a dodany ic26int-windows-x86_64.exe.
Windows prevod LF na CRLF byl pri porovnani vracen zpet; raw vystupy zustavaji v results/.

Vysledek: 319 PASS, 31 FAIL z 350 pripadu.

Toto je snimek tohoto behu, nikoli slib vysledku po dalsich zmenach prekladace.
Ocekavani nebyla prizpusobovana vystupum prekladace. Nesoulady je potreba projit podle zdroje a specifikace.

| Pripad | Nesoulad | Pozadavek |
|---|---|---|
| [base_semicolon_separators](cases/base/base_semicolon_separators.ifj) | compiler status 2; expected 0 | §1–4 |
| [base_contextual_words_are_identifiers](cases/base/base_contextual_words_are_identifiers.ifj) | compiler status 2; expected 0 | §1–4 |
| [base_division_truncates_to_zero](cases/base/base_division_truncates_to_zero.ifj) | program stdout differs | §1.1, §3; numeric-separator forum clarification |
| [base_division_negative_divisor](cases/base/base_division_negative_divisor.ifj) | program stdout differs | §1.1, §3; numeric-separator forum clarification |
| [base_read_int_invalid](cases/base/base_read_int_invalid.ifj) | program stdout differs | §4 |
| [base_read_int_whitespace](cases/base/base_read_int_whitespace.ifj) | program stdout differs | §4 |
| [base_read_int_underscore](cases/base/base_read_int_underscore.ifj) | program stdout differs | §4 |
| [base_read_int_eof](cases/base/base_read_int_eof.ifj) | program stdout differs | §4 |
| [base_read_double_invalid](cases/base/base_read_double_invalid.ifj) | runtime status 53; expected 0 | §4 |
| [base_read_double_whitespace](cases/base/base_read_double_whitespace.ifj) | runtime status 53; expected 0 | §4 |
| [base_read_double_eof](cases/base/base_read_double_eof.ifj) | runtime status 53; expected 0 | §4 |
| [control_if_expression_mixed_numeric](cases/control/control_if_expression_mixed_numeric.ifj) | compiler status 6; expected 0 | §5.3 |
| [control_if_inline_statement](cases/control/control_if_inline_statement.ifj) | compiler status 2; expected 0 | §5.3 |
| [control_for_negative_bounds](cases/control/control_for_negative_bounds.ifj) | compiler status 2; expected 0 | §5.3 |
| [control_for_guard](cases/control/control_for_guard.ifj) | compiler status 2; expected 0 | §5.3 |
| [control_for_two_guards](cases/control/control_for_two_guards.ifj) | compiler status 2; expected 0 | §5.3 |
| [control_for_inline_body](cases/control/control_for_inline_body.ifj) | compiler status 2; expected 0 | §5.3 |
| [control_for_guard_order](cases/control/control_for_guard_order.ifj) | compiler status 2; expected 0 | §5.3 |
| [control_if_expression_incompatible_branches](cases/control/control_if_expression_incompatible_branches.ifj) | compiler status 0; expected 6 | §5.3 |
| [control_for_guard_type](cases/control/control_for_guard_type.ifj) | compiler status 2; expected 6 | §5.3 |
| [control_for_literal_zero_step](cases/control/control_for_literal_zero_step.ifj) | compiler status 0; expected 10 | §5.3 |
| [algebdt_undefined_alias](cases/algebdt/algebdt_undefined_alias.ifj) | compiler status 6; expected 3 | §5.5 |
| [lists_for_list_guard](cases/lists/lists_for_list_guard.ifj) | compiler status 2; expected 0 | §5.8 |
| [lists_index_wrong_argument_type](cases/lists/lists_index_wrong_argument_type.ifj) | compiler status 6; expected 5 | §5.8 |
| [lists_drop_wrong_argument_type](cases/lists/lists_drop_wrong_argument_type.ifj) | compiler status 6; expected 5 | §5.8 |
| [lists_list_unknown_element_type](cases/lists/lists_list_unknown_element_type.ifj) | compiler status 0; expected 3 | §5.8 |
| [invalid_duplicate_parameter](cases/invalid/invalid_duplicate_parameter.ifj) | compiler status 0; expected 4 | §1–3, §6 |
| [invalid_boolean_arithmetic](cases/invalid/invalid_boolean_arithmetic.ifj) | compiler status 99; expected 6 | §1–3, §6 |
| [combined_showcase](cases/combined/combined_showcase.ifj) | compiler status 2; expected 0 | §5: meaningful combinations of implemented extensions |
| [combined_match_with_if_expression](cases/combined/combined_match_with_if_expression.ifj) | compiler status 6; expected 0 | §5: meaningful combinations of implemented extensions |
| [combined_match_guard_and_if_expression](cases/combined/combined_match_guard_and_if_expression.ifj) | compiler status 6; expected 0 | §5: meaningful combinations of implemented extensions |

## Rozdily v textovem vystupu

- base_division_truncates_to_zero: expected b'-3\n'; actual b'-2\n'
- base_division_negative_divisor: expected b'-3\n'; actual b'-2\n'
- base_read_int_invalid: expected b'0\n'; actual b'nil\n'
- base_read_int_whitespace: expected b'0\n'; actual b'nil\n'
- base_read_int_underscore: expected b'0\n'; actual b'nil\n'
- base_read_int_eof: expected b'0\n'; actual b'nil\n'
