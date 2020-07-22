# Instalación

Acceder a la carpeta node, que se encuentra dentro del repositorio, desde ahí se trabaja.

Una vez dentro de la carpeta node:

    Instalar las dependencias de Python (Python 3) con pip install -r requirements.txt

    Instalar dependencias de Node con npm install 

    Guardar los audios para el benchmark en la carpeta node/benchmark_comms

    Ejecutar el script BenchRaul.sh, que se encuentra en la carpeta Node.

    Si al ejecutar da fallo de versión de Node, ejecutar lo siguiente:

        $(npm bin)/electron-rebuild

# Ejecución de pruebas

## Evaluación de comunicaciones (microfono no funcional)

Ejecutar ./benchmark_comms/BigBenchmark.sh ACTION
Ahora solo funciona con la ACTION play

## Evaluación OpenMP (no funcional)
./benchmark_parallel.sh


## Evaluación OpenCL (GUI)
./benchmark_ocl.sh
