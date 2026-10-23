## Ejemplos de código para presentación de Collision Detection

### Introducción

Se implementan 3 programitas compilados con cmake.

Este repositorio utiliza submodules así que para clonarlo recomiendo que lo hagan recursivamente.

```
git clone --recurse-submodules https://github.com/francocampa/CollisionDetection
```

Luego la compilación con cmake:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build   
```

Esto va a generar 3 ejecutables:

- broad_phase
- mid_narrow_phase
- raycasting

### Controles

El movimiento de la cámara es el por defecto de Raylib: **wasd** para el movimiento con respecto hacia donde miramos, **espacio y ctrl** para subir y bajar.
Con **click derecho** toggleamos el movimiento de la cámara para poder interactuar con la ui

### Broad phase

Implementa una simulación de N esferas dentro de una caja cúbica para ejemplificar el uso de **spatial hashing** en la broad phase de detección de colisiones.

Tiene una pequeña ui para ajustar el número de esferas, tamaño de la caja, si se aplica optimización con grillas, y si se renderiza la escena (aunque no se renderice se sigue haciendo la simulación de fondo)

### Mid Narrow Phase

Implementa un pequeño sandbox donde agregar algunas mallas de triánguos para ver el impacto de la optimización de las fases mid y narrow en detección de colisiones.

Tiene una pequeña ui para agregar objetos, estos se pueden mover manteniendo apretado con el mouse y con la rueda (el movimiento es funcional, no cómodo)
La ui tiene opciones para activar optimización con bvh y visualizaciones de bvh.

### Raycasting

Implementa un pequeño sistema de movimiento para un personaje cuyas colisiones con el piso son estimadas usando raycasting.

El movimiento del personaje es con las **flechas** y salta con la **z** (si spamean el salto cae lento pero así es la vida)

No dio tiempo de implementar una cámara que siga al jugador así que se lo mira desde la distancia como papá orgulloso. 