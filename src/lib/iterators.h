/*
    Car controls.
    Copyright (C) 2015-16 Igor Stoppa <igor.stoppa@gmail.com>

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#ifndef ITERATORS_H
#define ITERATORS_H

#define VA_NARGS_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N
#define VA_NARGS(...) \
  VA_NARGS_IMPL(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

#define merge(a, b) a##b
#define dereference_and_merge(a, b) merge(a, b)

#define iterate_prepend_macro_prefix ITERATE_PREPEND_

#define ITERATE_PREPEND_1(prefix, parameter)      \
  prefix##parameter

#define ITERATE_PREPEND_2(prefix, parameter, ...) \
  prefix##parameter, \
  ITERATE_PREPEND_1(prefix, __VA_ARGS__)

#define ITERATE_PREPEND_3(prefix, parameter, ...) \
  prefix##parameter, \
  ITERATE_PREPEND_2(prefix, __VA_ARGS__)

#define ITERATE_PREPEND_4(prefix, parameter, ...) \
  prefix##parameter, \
  ITERATE_PREPEND_3(prefix,  __VA_ARGS__)

#define ITERATE_PREPEND_5(prefix, parameter, ...) \
  prefix##parameter, \
  ITERATE_PREPEND_4(prefix,  __VA_ARGS__)

#define ITERATE_PREPEND_6(prefix, parameter, ...) \
  prefix##parameter, \
  ITERATE_PREPEND_5(prefix,  __VA_ARGS__)

#define ITERATE_PREPEND_7(prefix, parameter, ...) \
  prefix##parameter, \
  ITERATE_PREPEND_6(prefix,  __VA_ARGS__)

#define ITERATE_PREPEND_8(prefix, parameter, ...) \
  prefix##parameter, \
  ITERATE_PREPEND_7(prefix,  __VA_ARGS__)

#define ITERATE_PREPEND_9(prefix, parameter, ...) \
  prefix##parameter, \
  ITERATE_PREPEND_8(prefix,  __VA_ARGS__)

#define ITERATE_PREPEND_10(prefix, parameter, ...) \
  prefix##parameter, \
  ITERATE_PREPEND_9(prefix,  __VA_ARGS__)

#define ITERATE_PREPEND_11(prefix, parameter, ...) \
  prefix##parameter, \
  ITERATE_PREPEND_10(prefix,  __VA_ARGS__)

#define ITERATE_PREPEND(prefix, ...) \
  dereference_and_merge(iterate_prepend_macro_prefix, \
                        VA_NARGS(__VA_ARGS__))(prefix, __VA_ARGS__)


#define iterate_comma_macro_prefix ITERATE_COMMA_

#define ITERATE_COMMA_1(action, parameter)      \
  action(parameter),

#define ITERATE_COMMA_2(action, parameter, ...) \
  action(parameter),                      \
  ITERATE_COMMA_1(action, __VA_ARGS__)

#define ITERATE_COMMA_3(action, parameter, ...) \
  action(parameter),                      \
  ITERATE_COMMA_2(action, __VA_ARGS__)

#define ITERATE_COMMA_4(action, parameter, ...) \
  action(parameter),                      \
  ITERATE_COMMA_3(action,  __VA_ARGS__)

#define ITERATE_COMMA_5(action, parameter, ...) \
  action(parameter),                      \
  ITERATE_COMMA_4(action,  __VA_ARGS__)

#define ITERATE_COMMA_6(action, parameter, ...) \
  action(parameter),                    \
  ITERATE_COMMA_5(action,  __VA_ARGS__)

#define ITERATE_COMMA_7(action, parameter, ...) \
  action(parameter),                      \
  ITERATE_COMMA_6(action,  __VA_ARGS__)

#define ITERATE_COMMA_8(action, parameter, ...) \
  action(parameter),                      \
  ITERATE_COMMA_7(action,  __VA_ARGS__)

#define ITERATE_COMMA_9(action, parameter, ...) \
  action(parameter),                      \
  ITERATE_COMMA_8(action,  __VA_ARGS__)

#define ITERATE_COMMA_10(action, parameter, ...) \
  action(parameter),                      \
  ITERATE_COMMA_9(action,  __VA_ARGS__)

#define ITERATE_COMMA_11(action, parameter, ...) \
  action(parameter),                      \
  ITERATE_COMMA_10(action,  __VA_ARGS__)

#define ITERATE_COMMA(action, ...) \
  dereference_and_merge(iterate_comma_macro_prefix, \
                        VA_NARGS(__VA_ARGS__))(action, __VA_ARGS__)

#define iterate_semicolon_macro_prefix ITERATE_SEMICOLON_

#define ITERATE_SEMICOLON_1(action, parameter)      \
  action(parameter);

#define ITERATE_SEMICOLON_2(action, parameter, ...) \
  action(parameter);                      \
  ITERATE_SEMICOLON_1(action, __VA_ARGS__)

#define ITERATE_SEMICOLON_3(action, parameter, ...) \
  action(parameter);                      \
  ITERATE_SEMICOLON_2(action, __VA_ARGS__)

#define ITERATE_SEMICOLON_4(action, parameter, ...) \
  action(parameter);                      \
  ITERATE_SEMICOLON_3(action,  __VA_ARGS__)

#define ITERATE_SEMICOLON_5(action, parameter, ...) \
  action(parameter);                      \
  ITERATE_SEMICOLON_4(action,  __VA_ARGS__)

#define ITERATE_SEMICOLON_6(action, parameter, ...) \
  action(parameter);                    \
  ITERATE_SEMICOLON_5(action,  __VA_ARGS__)

#define ITERATE_SEMICOLON_7(action, parameter, ...) \
  action(parameter);                      \
  ITERATE_SEMICOLON_6(action,  __VA_ARGS__)

#define ITERATE_SEMICOLON_8(action, parameter, ...) \
  action(parameter);                      \
  ITERATE_SEMICOLON_7(action,  __VA_ARGS__)

#define ITERATE_SEMICOLON_9(action, parameter, ...) \
  action(parameter);                      \
  ITERATE_SEMICOLON_8(action,  __VA_ARGS__)

#define ITERATE_SEMICOLON_10(action, parameter, ...) \
  action(parameter);                      \
  ITERATE_SEMICOLON_9(action,  __VA_ARGS__)

#define ITERATE_SEMICOLON_11(action, parameter, ...) \
  action(parameter);                      \
  ITERATE_SEMICOLON_10(action,  __VA_ARGS__)

#define ITERATE_SEMICOLON(action, ...) \
  dereference_and_merge(iterate_semicolon_macro_prefix, \
                        VA_NARGS(__VA_ARGS__))(action, __VA_ARGS__)

#endif
