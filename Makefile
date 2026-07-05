# Variáveis de compilação
CC = gcc
CFLAGS = -Wall -Wextra -g -O2 -pthread -Iinclude

# Nome do executável exigido pela especificação
TARGET = trabSO

# Diretórios do projeto
SRC_DIR = src
BUILD_DIR = build

# Coleta os arquivos fonte e define os arquivos objeto dentro de build/
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

.PHONY: all monoprocessador multiprocessador clean

# Alvo padrão (Seção 4.3)
all: monoprocessador

monoprocessador: CFLAGS += -UVERSION_MULTIPROCESSOR
monoprocessador: clean $(TARGET)
	@echo "Mini-Kernel Monoprocessador compilado com sucesso como './$(TARGET)'."

multiprocessador: CFLAGS += -DVERSION_MULTIPROCESSOR
multiprocessador: clean $(TARGET)
	@echo "Mini-Kernel Multiprocessador compilado com sucesso como './$(TARGET)'."

# Linkagem do executável final na raiz do projeto
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# Regra de compilação dos objetos (cria a pasta build se ela não existir)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Limpeza completa e cirúrgica
clean:
	rm -rf $(BUILD_DIR) $(TARGET) log_execucao_minikernel.txt