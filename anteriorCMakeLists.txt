cmake_minimum_required(VERSION 3.10)
project(minikernel C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Flags de compilação rigorosas para evitar erros comuns de ponteiro e concorrência
add_compile_options(-Wall -Wextra -g -O2)

# Busca obrigatoriamente a biblioteca pthread do POSIX
find_package(Threads REQUIRED)

# Inclui o diretório de cabeçalhos
include_directories(include)

# Coleta todos os arquivos fonte
file(GLOB SOURCES "src/*.c")

# Define o executável com o nome exigido pela especificação: trabSO
add_executable(trabSO ${SOURCES})

# =========================================================================
# CORREÇÃO: Mapeia o parâmetro -DUSE_MULTIPROCESSOR=ON para a macro do C
# =========================================================================
if(USE_MULTIPROCESSOR)
    target_compile_definitions(trabSO PRIVATE VERSION_MULTIPROCESSOR)
endif()

# Linka com a biblioteca de threads do sistema
target_link_libraries(trabSO PRIVATE Threads::Threads)