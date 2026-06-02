#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/codegen.h"

void printUsage() {
    std::cerr << "Usage: twit <file.twit> [-o output]" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string inputFile;
    std::string outputFile;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-o") {
            if (i + 1 < argc) outputFile = argv[++i];
            else { printUsage(); return 1; }
        } else {
            inputFile = arg;
        }
    }

    if (inputFile.empty()) {
        printUsage();
        return 1;
    }

    // 출력 파일명 자동 설정
    if (outputFile.empty()) {
        outputFile = inputFile.substr(0, inputFile.find_last_of('.'));
    }

    // 파일 읽기
    std::ifstream file(inputFile);
    if (!file.is_open()) {
        std::cerr << "Error: cannot open file " << inputFile << std::endl;
        return 1;
    }
    std::stringstream ss;
    ss << file.rdbuf();
    std::string source = ss.str();

    try {
        // Lexer
        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        // Parser
        Parser parser(tokens);
        auto program = parser.parse();

        // CodeGen
        CodeGen codegen;
        codegen.generate(program);

        std::string llFile = outputFile + ".ll";
        std::string sFile  = outputFile + ".s";
        codegen.writeToFile(llFile);

        // llc & clang
        std::string llcCmd   = "llc -relocation-model=pic " + llFile + " -o " + sFile;
        std::string clangCmd = "clang -fPIE -pie " + sFile + " -o " + outputFile + " -lm -lgc";
        system(llcCmd.c_str());
        system(clangCmd.c_str());

        // 중간 파일 삭제
        remove(llFile.c_str());
        remove(sFile.c_str());

        std::cout << "Compiled: " << outputFile << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}