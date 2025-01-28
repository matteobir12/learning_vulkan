CFLAGS = -std=c++17 -Og
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
TARGET = SimpleVulkanRenderer
SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(SOURCES:.cpp=.o)

all: shaders $(TARGET)

.PHONY: shaders test clean

shaders:
	cd shaders && ./compile.sh

$(TARGET): $(OBJECTS)
	g++ $(CFLAGS) $(OBJECTS) -o build/$(TARGET) $(LDFLAGS)

%.o: %.cpp
	g++ $(CFLAGS) -c $< -o build/$@


test: SimpleVulkanRenderer
	./build/SimpleVulkanRenderer

clean:
	rm -f build/*