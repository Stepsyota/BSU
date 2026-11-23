CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -O2 -fPIC
INCLUDES := -I src/domain/include -I src/io/include -I src/ui/include

QT_CFLAGS := $(shell pkg-config --cflags Qt6Widgets)
QT_LIBS   := $(shell pkg-config --libs Qt6Widgets)
MOC       := $(shell pkg-config --variable=libexecdir Qt6Core)/moc

BUILD  := build
OBJ    := $(BUILD)/obj
MOCDIR := $(BUILD)/moc
TARGET := $(BUILD)/elevators

DOMAIN_SRCS := \
	src/domain/src/Person.cpp \
	src/domain/src/Elevator.cpp \
	src/domain/src/MinWaitStrategy.cpp \
	src/domain/src/MinIdleStrategy.cpp \
	src/domain/src/Scenario.cpp \
	src/domain/src/Simulation.cpp

IO_SRCS := src/io/src/JsonStore.cpp

UI_SRCS := \
	src/ui/src/MainWindow.cpp \
	src/ui/src/ConfigDialog.cpp \
	src/ui/src/StatsDialog.cpp

APP_SRCS := main.cpp

QT_HDRS := \
	src/ui/include/MainWindow.h \
	src/ui/include/ConfigDialog.h \
	src/ui/include/StatsDialog.h

CPP_SRCS := $(DOMAIN_SRCS) $(IO_SRCS) $(UI_SRCS) $(APP_SRCS)
CPP_OBJS := $(patsubst %.cpp,$(OBJ)/%.o,$(CPP_SRCS))
MOC_SRCS := $(patsubst src/ui/include/%.h,$(MOCDIR)/moc_%.cpp,$(QT_HDRS))
MOC_OBJS := $(patsubst $(MOCDIR)/%.cpp,$(OBJ)/%.o,$(MOC_SRCS))
OBJS     := $(CPP_OBJS) $(MOC_OBJS)

.PHONY: all clean run help
.SECONDARY: $(MOC_SRCS)

all: $(TARGET)

help:
	@echo "Цели:"
	@echo "  make / make all  — собрать $(TARGET)"
	@echo "  make run         — собрать и запустить"
	@echo "  make clean       — удалить каталог $(BUILD)"
	@echo "  make help        — показать эту справку"

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(QT_LIBS)

$(MOCDIR)/moc_%.cpp: src/ui/include/%.h
	@mkdir -p $(dir $@)
	$(MOC) $< -o $@

$(OBJ)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(QT_CFLAGS) -c $< -o $@

$(OBJ)/moc_%.o: $(MOCDIR)/moc_%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(QT_CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD)
