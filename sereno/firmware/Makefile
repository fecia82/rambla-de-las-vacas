# Pruebas y simulación del núcleo de Sereno en un PC (no hace falta la placa).
#   make prueba   compila y ejecuta las pruebas
#   make sim      simula un invierno y un verano y escribe docs/simulacion.json
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wno-sign-conversion
NUCLEO   := $(wildcard lib/sereno_core/*.cpp)
SALIDA   := .pc

.PHONY: prueba sim limpia

prueba: $(SALIDA)/pruebas
	./$(SALIDA)/pruebas

sim: $(SALIDA)/simulacion
	./$(SALIDA)/simulacion ../docs/simulacion.json

$(SALIDA)/pruebas: pruebas_pc/pruebas.cpp $(NUCLEO) $(wildcard lib/sereno_core/*.h)
	@mkdir -p $(SALIDA)
	$(CXX) $(CXXFLAGS) -Ilib/sereno_core -o $@ pruebas_pc/pruebas.cpp $(NUCLEO)

$(SALIDA)/simulacion: pruebas_pc/simulacion.cpp $(NUCLEO) $(wildcard lib/sereno_core/*.h)
	@mkdir -p $(SALIDA)
	$(CXX) $(CXXFLAGS) -Ilib/sereno_core -o $@ pruebas_pc/simulacion.cpp $(NUCLEO)

limpia:
	rm -rf $(SALIDA)
