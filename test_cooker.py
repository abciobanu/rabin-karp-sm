from argparse import ArgumentParser
from random import randint, choices
from string import ascii_letters
from tqdm import tqdm


def text_generator(min_size: int, max_size: int) -> str:
    """
    Generates a string from where we'll add patterns later.
    :param min_size: The minimum size of the text.
    :param max_size: The maximum size of the text.
    :return: A string with length between min_size and max_size (randomly chosen).
    """

    length = randint(min_size, max_size)

    return ''.join(choices(ascii_letters + ' ', k=length))


def main():
    parser = ArgumentParser()
    parser.add_argument('-num_tests', '--n', default=10, type=int)
    parser.add_argument('-min_text_size', '--min', default=1000, type=int)
    parser.add_argument('-max_text_size', '--max', default=2000, type=int)
    args = parser.parse_args()

    for _ in tqdm(range(args.n), desc='generating tests'):
        text = text_generator(args.min, args.max)


if __name__ == '__main__':
    main()
