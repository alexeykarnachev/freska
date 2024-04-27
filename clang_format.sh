#!/bin/bash
clang-format -style=file -i `find src -name "*.cpp"`
clang-format -style=file -i `find src -name "*.hpp"`
