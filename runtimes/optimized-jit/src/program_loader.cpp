#include "program_loader.hpp"

uint64_t ProgramLoader::make_word(uint64_t word, unsigned width, unsigned lsb,
                   uint64_t value) {
    uint64_t mask = (uint64_t)1 << (width - 1);
    mask = mask << 1;
    mask -= 1;
    mask = mask << lsb;
    mask = ~mask;

    uint64_t new_word = (word & mask);
    value = value << lsb;
    uint64_t return_word = (new_word | value);
    return return_word;
}

ProgramLoader::ProgramLoader() {
    x = 0;
    program = {};
}

void ProgramLoader::load_file(std::ifstream &in)
{
    uint32_t word;
    int c;
    int i = 0;
    char c_char;
    size_t offset;
    unsigned char ch;
    while (in.get(c_char)) {
        ch = (unsigned char)c_char;
        if (i % 4 == 0)
            word = make_word(word, 8, 24, ch);
        else if (i % 4 == 1)
            word = make_word(word, 8, 16, ch);
        else if (i % 4 == 2)
            word = make_word(word, 8, 8, ch);
        else if (i % 4 == 3)
        {
            word = make_word(word, 8, 0, ch);
            program.push_back(word);

            /* Storing the UM word in the zero segment */
            // set_at(umem, 0 + (i / 4) * sizeof(uint32_t), word);

            /* Compiling the UM word into machine code */
            // offset = compile_instruction(zero, word, offset);
            // This is probably where the compiling step wants to go,
            // but we will consider this later
            word = 0;
        }

        i++;
    }
}