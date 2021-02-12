#!/bin/sh
# Copyright 2021 gRPC authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -e

cd "$(dirname "$0")/../../.."

#
# Disallow the usage of certain terms.
#

grep -PIirn \
    '((\b|_)(black[\ -]?hat|black[\ -]?list|black[\ -]?listed|black[\ -]?listing|dummy|grand[\ -]?father\ clause|grand[\ -]?fathered|hang|hung|man[\ -]?power|man[\ -]?hours|master(?!/)|slave|white[\ -]?hat|white[\ -]?list|white[\ -]?listed|white[\ -]?listing)(\b|_))' \
    examples \
    include \
    src/abseil-cpp \
    src/android \
    src/core \
    src/cpp \
    test | \
    diff - /dev/null


grep -PIirn '((\b|_)(ADHD|agnostic|blast[\ -]?radius|build\ cop|build\ sheriff|dead[\ -]?letter\ queue|dead[\ -]?letter|demilitarized\ zone\ (DMZ)|demilitarized\ zone|DMZ|derpy|dojo|fat|female\ adapter|field[\ -]?worker|first[\ -]?class|ghetto|grayed[\ -]?out|greyed[\ -]?out|gray[\ -]?out|grey[\ -]?out|gray[\ -]?list|grey[\ -]?list|gray[\ -]?listed|grey[\ -]?listed|gray[\ -]?listing|grey[\ -]?listing|gray[\ -]?hat|grey[\ -]?hat|guru|hit|house[\ -]?keeping|inscrutable|kebab|kebab[\ -]?case|kabob[\ -]?case|kabob|kill(?!\()|drink\ the\ Kool[\ -]?Aid|Kool\ Aid|life[\ -]?support|male\ adapter|manned|man[\ -]?kind|man[\ -]?made|mom\ test|girl[\ -]?friend\ test|grand[\ -]?ma\ test|grand[\ -]?mother\ test|ninja|OCD|off\ the\ reservation|open[\ -]?the[\ -]?kimono|pets[\ -]?versus[\ -]?cattle|preferred\ pronouns|RTFM|sane|sanity[\ -]?check|STONITH|STOMITH|terminate|tribal\ knowledge|tribal\ wisdom|voodoo|white[\ -]?glove|\bblast-radius\b|\bwhite-glove\b)(\b|_))' \
    examples/cpp \
    include \
    src/core \
    src/cpp \
    test | \
    diff - /dev/null
