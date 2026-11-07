# ==============================================================
#  Makefile - Simulador de Kernel (Fase I)
#  Uso:
#    make          -> compila version normal  (./simulador)
#    make run      -> compila y ejecuta
#    make debug    -> compila con AddressSanitizer + UBSan (detecta segfaults)
#    make tsan     -> compila con ThreadSanitizer (detecta condiciones de carrera)
#    make clean    -> borra todo lo compilado
# ==============================================================

# Las recetas usan '>' en vez de TAB: asi no se rompe al copiar/pegar.
.RECIPEPREFIX := >

CXX      := g++
TARGET   := simulador
SRC_DIR  := src
INC_DIR  := include
OBJ_DIR  := build

CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -I$(INC_DIR) -pthread -MMD -MP
LDFLAGS  := -pthread

MODE ?= release
ifeq ($(MODE),debug)
  CXXFLAGS += -O0 -g -fsanitize=address,undefined -fno-omit-frame-pointer
  LDFLAGS  += -fsanitize=address,undefined
else ifeq ($(MODE),tsan)
  CXXFLAGS += -O1 -g -fsanitize=thread
  LDFLAGS  += -fsanitize=thread
else
  CXXFLAGS += -O2
endif

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all run debug tsan clean

all: $(TARGET)

$(TARGET): $(OBJS)
> $(CXX) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
> $(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
> mkdir -p $(OBJ_DIR)

run: all
> ./$(TARGET)

debug:
> $(MAKE) clean
> $(MAKE) MODE=debug

tsan:
> $(MAKE) clean
> $(MAKE) MODE=tsan

clean:
> rm -rf $(OBJ_DIR) $(TARGET)

-include $(DEPS)
