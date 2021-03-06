<?php
# Check cases where LIT_OPTS has no effect.
#
# RUN:                 %{phplit} -j 1 -s %{inputs}/lit-opts | filechecker %s
# RUN: env LIT_OPTS=   %{phplit} -j 1 -s %{inputs}/lit-opts | filechecker %s
# RUN: env LIT_OPTS=-s %{phplit} -j 1 -s %{inputs}/lit-opts | filechecker %s

# Check that LIT_OPTS can override command-line options.
#
# RUN: env LIT_OPTS=-a \
# RUN: %{phplit} -j 1 -s %{inputs}/lit-opts \
# RUN: | filechecker --check-prefix=SHOW-ALL -DVAR= %s

# Check that LIT_OPTS understands multiple options with arbitrary spacing.
#
# RUN: env LIT_OPTS='-a -v  -Dvar=foobar' \
# RUN: %{phplit} -j 1 -s %{inputs}/lit-opts \
# RUN: | filechecker --check-prefix=SHOW-ALL -DVAR=foobar %s

# Check that LIT_OPTS parses shell-like quotes and escapes.
#
# RUN: env LIT_OPTS='-a   -v -Dvar="foo bar"\ baz' \
# RUN: %{phplit} -j 1 -s %{inputs}/lit-opts \
# RUN: | filechecker --check-prefix=SHOW-ALL -DVAR="foo bar baz" %s

# CHECK:      Testing: 1 tests
# CHECK-NOT:  PASS
# CHECK:      Expected Passes : 1

# SHOW-ALL:     Testing: 1 tests
# SHOW-ALL:     PASS: lit-opts :: test.txt (1 of 1)
# SHOW-ALL:     {{^}}[[VAR]]
# SHOW-ALL-NOT: PASS
# SHOW-ALL:     Expected Passes : 1
