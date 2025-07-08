#pragma once
#include <vector>
#include <iostream>
#include <fstream>

class ProgramLoader {
    private:
        int x;
        uint64_t make_word(uint64_t word, unsigned width, unsigned lsb,
                   uint64_t value);

    public:
        std::vector<uint32_t> program;
        ProgramLoader();
        void load_file(std::ifstream &in);

};