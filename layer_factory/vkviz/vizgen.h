#ifndef VIZGEN_H
#define VIZGEN_H

#include "command_enums.h"
#include "command_buffer.h"

#include <fstream>
#include <string>

class Generator {
    std::ofstream& out_file_;

 public:
    // Note that the given outfile must outlive the constructed Generator
    Generator(std::ofstream& out_file) : out_file_(out_file) {}

    void Write(const VkVizCommandBuffer& command_buffer) {
        out_file_ << "<table>" << std::endl;

        for(const auto& command : command_buffer.Commands()) {
            out_file_ << "<tr><td>" << command.TypeString() << "</td></tr>" << std::endl;
        }

        out_file_ << "</table>" << std::endl;
    }
};

#endif  // VIZGEN_H
