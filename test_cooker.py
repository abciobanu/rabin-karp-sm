import os
import re
import signal
from argparse import ArgumentParser
from random import randint, choices
from string import ascii_letters
from pathlib import Path
from pandas import Interval


# https://stackoverflow.com/a/22348885/14521651
# We need this because there is a super-edge case where the patterns generator
# fails to generate disjoint intervals...
class timeout:
    def __init__(self, seconds=1, error_message='Timeout'):
        self.seconds = seconds
        self.error_message = error_message

    def handle_timeout(self, signum, frame):
        raise TimeoutError(self.error_message)

    def __enter__(self):
        signal.signal(signal.SIGALRM, self.handle_timeout)
        signal.alarm(self.seconds)

    def __exit__(self, type, value, traceback):
        signal.alarm(0)


def text_generator(min_size: int, max_size: int) -> str:
    """
    Generates a string from where we'll add patterns later.
    :param min_size: The minimum size of the text.
    :param max_size: The maximum size of the text.
    :return: A string with length between min_size and max_size (randomly chosen).
    """

    length = randint(min_size, max_size)

    return ''.join(choices(ascii_letters + ' ', k=length))


def patterns_generator(text: str,
                       min_pattern_size: int,
                       max_pattern_size: int,
                       num_patterns: int,
                       min_insertions: int,
                       max_insertions: int
                       ) -> (str, list, list):
    """
    Inserts random generated patterns in the given text.
    :param text: The text where to insert patterns.
    :param min_pattern_size: The minimum size of a pattern.
    :param max_pattern_size: The maximum size of a pattern.
    :param num_patterns: The number of patterns to insert.
    :param min_insertions: The minimum number of insertions.
    :param max_insertions: The maximum number of insertions.
    :return: The text with the inserted patterns, the list of the patterns and
    how many times they were inserted.
    """

    start_indexes = set()
    occupied_text_ranges = set()

    patterns = [text_generator(min_pattern_size, max_pattern_size) for _ in range(num_patterns)]
    insertion_counter = []
    max_generated_pattern_len = max(list(map(len, patterns)))

    for pattern in patterns:
        num_insertions = randint(min_insertions, max_insertions)
        insertion_counter.append(num_insertions)
        print(f"Will insert pattern {pattern} {num_insertions} times.")

        i = 0
        while i < num_insertions:
            start = randint(0, len(text) - max_generated_pattern_len)
            while start in start_indexes:
                start = randint(0, len(text) - max_generated_pattern_len)
            end = start + len(pattern)

            crt_range = Interval(start, end)
            ok = True
            for text_range in occupied_text_ranges:
                if crt_range.overlaps(text_range):
                    ok = False
                    break

            if not ok:
                continue

            i += 1
            occupied_text_ranges.add(crt_range)
            text = text[:start] + pattern + text[end:]

        print(f"Insertion successful!")

    return text, patterns, insertion_counter


def write_files(fname: str, text: str, patterns: list, insertion_counter: list) -> None:
    """
    Writes the input and ref files.
    :param fname: The basename for files.
    :param text: The text.
    :param patterns: The patterns.
    :param insertion_counter: The number of times the patterns have been inserted.
    :return: Nothing, just creates the files
    """

    with open(f'{fname}.in', 'w') as handle:
        handle.write(str(len(patterns)) + '\n')
        for pattern in patterns:
            handle.write(pattern + '\n')
        handle.write(text + '\n')

    with open(f'{fname}.ref', 'w') as handle:
        for pattern, ins_cnt in zip(patterns, insertion_counter):
            txt = pattern + ':'
            num_findings = 0
            for match in re.finditer(pattern, text):
                txt += ' ' + str(match.start())
                num_findings += 1

            # There can be more findings if the random text generator has generated
            # the pattern somewhere in the string.
            if num_findings < ins_cnt:
                print(f"WARNING: insertion in text failed a little for pattern {pattern}")

            handle.write(txt + '\n')


def main():
    parser = ArgumentParser()

    parser.add_argument('-output_folder', '--o', default='tests', type=str)

    parser.add_argument('-num_tests', '--n', default=10, type=int)
    parser.add_argument('--min_test', default=1, type=int)
    parser.add_argument('--max_test', default=10, type=int)

    parser.add_argument('--min_text', default=1000, type=int)
    parser.add_argument('--max_text', default=100000, type=int)

    parser.add_argument('--min_pattern', default=10, type=int)
    parser.add_argument('--max_pattern', default=80, type=int)

    parser.add_argument('--min_insertions', default=0, type=int)
    parser.add_argument('--max_insertions', default=10, type=int)

    args = parser.parse_args()

    Path.mkdir(Path(args.o), exist_ok=True)

    for i in range(args.n):
        print('=' * 5 + f'Generating test {i}' + '=' * 5)
        text = text_generator(args.min_text, args.max_text)
        num_patterns = randint(args.min_test, args.max_test)

        with timeout(seconds=10):
            text, patterns, ins_counter = patterns_generator(
                text,
                args.min_pattern,
                args.max_pattern,
                num_patterns,
                args.min_insertions,
                args.max_insertions
            )

        fname = os.path.join(args.o, f'test{i}')
        write_files(fname, text, patterns, ins_counter)


if __name__ == '__main__':
    main()
