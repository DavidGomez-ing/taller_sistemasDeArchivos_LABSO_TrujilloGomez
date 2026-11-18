# Bitácora – Taller: sistemas de archivos

**Laboratorio de Sistemas Operativos**

**Estudiante:** _(Joseph David Trujillo Gomez)_

**Entorno:** Debian GNU/Linux en VirtualBox 

> Idea que ordena el taller: **el nombre no es el archivo.** El archivo es la
> estructura de metadatos (inodo) que apunta a los bloques de datos; el
> directorio solo asocia un nombre con ella.

Toda la salida cruda de los comandos está en `evidencia/evidencia.txt`
(generada con `./correr_taller.sh`).

---

## Primera parte: observar el sistema

### Punto 1 – Sistemas de archivos montados

Resumen de lo observado (completar con la salida de `df -h`, `df -i`, `lsblk -f`):

| Dispositivo | Tipo | Tamaño | Libre | Inodos totales | Inodos libres | Montado en |
|---|---|---|---|---|---|---|
| /dev/sda1 | ext4 | 38G | 22G | 2485504 | 2275903 | `/` |
| tmpfs | tmpfs | 4,0G | 4,0G | 1024780 | 1024776 | `/dev/shm` |
| tmpfs | tmpfs | 4,0G | 4,0G | 1048576 | 1048542 | `/tmp` |

En `/` se usa el 41 % del espacio pero solo el 9 % de los inodos (209601 de
2485504), por eso hay margen en ambos recursos. Ese contraste muestra que
espacio e inodos se agotan de forma independiente.

### Punto 2 – ¿Qué informa `df -i`?

`df -h` informa el espacio en **bloques de datos**; `df -i` informa los
**inodos**: cuántos tiene el sistema de archivos en total, cuántos están en uso
y cuántos quedan libres. Cada archivo, directorio o enlace simbólico consume un
inodo (los metadatos) sin importar su tamaño.

En sistemas como ext4 la cantidad de inodos se fija al crear el sistema de
archivos. Si se crean muchísimos archivos pequeños (por ejemplo millones de
archivos de caché, sesiones o correos), se pueden **agotar los inodos antes que
los bloques**. Entonces `df -h` sigue mostrando espacio libre, pero crear
cualquier archivo nuevo falla con `No space left on device`, porque no hay
ninguna estructura de metadatos disponible para describirlo. El sistema queda
inutilizable para escritura aunque haya espacio.

### Punto 3 – Metadatos con `stat` y `ls -li`

`stat` muestra: tamaño, bloques, tipo, dispositivo, **inodo**, **contador de
enlaces**, permisos y propietario/grupo, y los tiempos de acceso (atime),
modificación (mtime) y cambio de metadatos (ctime). Observación importante:
`stat .` (el directorio) también tiene su propio inodo, tamaño, permisos y
tiempos, es decir, **un directorio también es un archivo**; su contenido es la
tabla de pares *nombre → número de inodo*.

Valores observados para `archivo.txt`: inodo = ____, enlaces = ____, tamaño = ____.

---

## Segunda parte: enlaces

### Punto 5 – Inodo y contador de enlaces (valores anotados)

Salida de `ls -li archivo.txt duro.txt simbolico.txt` en nuestra VM:

```
2249972 -rw-rw-r-- 2 ingesis ingesis 31 nov 18 11:51 archivo.txt
2249972 -rw-rw-r-- 2 ingesis ingesis 31 nov 18 11:51 duro.txt
2249973 lrwxrwxrwx 1 ingesis ingesis 11 nov 18 11:51 simbolico.txt -> archivo.txt
```

| Nombre | Inodo | Contador de enlaces | Tamaño | Tipo |
|---|---|---|---|---|
| `archivo.txt` | 2249972 | 2 | 31 bytes | archivo regular |
| `duro.txt` | 2249972 | 2 | 31 bytes | archivo regular |
| `simbolico.txt` | 2249973 | 1 | 11 bytes | enlace simbólico |

Análisis:

- `archivo.txt` y `duro.txt` tienen **el mismo inodo (2249972)** y contador de
  enlaces **2**: son dos nombres distintos para un único archivo. Por eso
  también comparten tamaño, permisos y fecha. El enlace duro **no es una
  copia**: no existen dos archivos, existe uno solo con dos nombres.
- `simbolico.txt` tiene **un inodo distinto (2249973)** y contador **1**: es un
  archivo propio, de tipo enlace simbólico, cuyo contenido es la ruta del
  destino.
- El **tamaño del enlace simbólico (11 bytes)** corresponde a la longitud de la
  ruta que guarda (`archivo.txt` tiene 11 caracteres), **no** al tamaño del
  archivo al que apunta (31 bytes: 30 caracteres de texto más el salto de línea).
- Sus permisos aparecen como `rwxrwxrwx`, pero no se usan: al acceder al
  contenido mandan los permisos del destino.
- Los permisos `rw-rw-r--` de los archivos regulares se leen por grupos de tres:
  el propietario (`ingesis`) puede leer y escribir, el grupo (`ingesis`) puede
  leer y escribir, y los demás solo pueden leer.

### Punto 6 – Borrar el archivo original

Se ejecutó `rm archivo.txt` y luego `ls -li duro.txt simbolico.txt`:

```
2249972 -rw-rw-r-- 1 ingesis ingesis 31 nov 18 11:51 duro.txt
2249973 lrwxrwxrwx 1 ingesis ingesis 11 nov 18 11:51 simbolico.txt -> archivo.txt
```

Resultado en cada enlace:

- **`duro.txt`** sigue funcionando: conserva el inodo 2249972 y su contador de
  enlaces bajó de **2 a 1**. `cat duro.txt` sigue mostrando `contenido de prueba
  del taller`. El comando `rm` no borra el archivo, borra *un nombre*: elimina
  esa entrada del directorio y resta 1 al contador de enlaces. Los bloques de
  datos y el inodo solo se liberan cuando el contador llega a 0 (y ningún
  proceso lo tiene abierto). Al quedar todavía un nombre (`duro.txt`), el
  archivo sigue existiendo.
- **`simbolico.txt`** quedó **roto** (enlace colgante): `ls -li` todavía lo
  muestra apuntando a `archivo.txt`, pero `cat simbolico.txt` falla con
  `No such file or directory`. El enlace simbólico guarda solo la
  ruta `archivo.txt`, y no se cuenta en el contador de enlaces del archivo. Al
  desaparecer ese nombre, la ruta ya no lleva a ningún sitio. Conserva su
  inodo (2249973) y su tamaño (11 bytes) porque ese enlace sigue siendo un
  archivo válido; lo que se perdió es el destino.

Conclusión: borrar un nombre no borra el archivo mientras queden enlaces duros
a él; un enlace simbólico, en cambio, depende de que el nombre al que apunta
siga existiendo.



### Punto 7 – Enlaces entre sistemas de archivos

Para tener dos sistemas de archivos distintos se compararon la carpeta de
trabajo y `/dev/shm`. Salida de `df --output=source,fstype,target . /dev/shm`:

```
Filesystem     Type  Mounted on
/dev/sda1      ext4  /
tmpfs          tmpfs /dev/shm
```

La carpeta de trabajo está en `/dev/sda1` (ext4) y `/dev/shm` es un sistema de
archivos distinto (tmpfs, en memoria).

**Enlace duro entre sistemas de archivos: falla.**

```
$ ln duro.txt /dev/shm/duro_otro_fs.txt
ln: failed to create hard link '/dev/shm/duro_otro_fs.txt' => 'duro.txt': Invalid cross-device link
```

**Enlace simbólico entre sistemas de archivos: funciona.**

```
$ ln -s "$PWD/duro.txt" /dev/shm/simbolico_otro_fs.txt
$ ls -li /dev/shm/simbolico_otro_fs.txt
5 lrwxrwxrwx 1 ingesis ingesis 40 nov 18 11:51 /dev/shm/simbolico_otro_fs.txt -> /home/ingesis/taller_fs/pruebas/duro.txt
$ cat /dev/shm/simbolico_otro_fs.txt
contenido de prueba del taller
```

Por qué el enlace duro no se puede crear:

- Una entrada de directorio es un par *nombre → número de inodo*, y ese número
  **solo tiene significado dentro de su propio sistema de archivos**. Cada
  sistema de archivos tiene su propia tabla de inodos y su propia numeración.
  Lo comprobamos con nuestros datos: `duro.txt` tiene el inodo **2249972** en
  `/dev/sda1`, mientras que el enlace creado en `/dev/shm` recibió el inodo **5**
  en tmpfs. Son numeraciones independientes que no guardan relación.
- Un directorio de `/dev/shm` no podría apuntar al inodo 2249972, porque en
  tmpfs ese número no identifica ningún archivo (o identificaría otro
  distinto). Por eso el kernel rechaza la operación con el error
  `Invalid cross-device link` (código `EXDEV`).

Por qué el enlace simbólico sí funciona:

- Un enlace simbólico **no guarda un número de inodo, guarda una ruta de
  texto**: aquí `/home/ingesis/taller_fs/pruebas/duro.txt`, que son 40 caracteres
  y coincide con el tamaño de 40 bytes que muestra `ls -li`.
- El destino se resuelve **en el momento de acceder**, recorriendo el árbol de
  directorios desde la raíz, y esa ruta puede cruzar puntos de montaje sin
  problema. Por eso `cat` sigue mostrando el contenido de `duro.txt` aunque el
  enlace viva en otro sistema de archivos.
- El enlace simbólico es un archivo nuevo, con su propio inodo dentro de
  tmpfs (el 5), y no comparte inodo con el destino. Por eso no tiene la
  restricción del alcance de los identificadores.

Conclusión: el alcance de un número de inodo es un solo sistema de archivos.
El enlace duro depende de ese número; el simbólico depende de un nombre.

## Tercera parte: programa en C

Archivo: `infoarchivo.c`. Compilación: `make` (usa `gcc -Wall -Wextra`, sin
advertencias).

Decisiones de diseño:

- Usa `lstat()` en lugar de `stat()`, para que sobre un enlace simbólico informe
  los metadatos **del enlace**. Si es un enlace, además muestra la ruta a la que
  apunta (`readlink`) y, con `stat()`, el tamaño del destino (o indica que está roto).
- Los permisos se muestran como cadena `rwxr-xr-x`, en octal y **separados por
  usuario**: propietario (con su nombre), grupo (con su nombre) y otros.
- Muestra también el inodo, útil para relacionarlo con el punto 5.

### Punto 9 – Resultados y diferencias

Se ejecutó `./infoarchivo` sobre un archivo normal, un directorio y un enlace
simbólico (salida completa en `evidencia/evidencia.txt`, sección *PARTE 3*).
Valores obtenidos en nuestra VM:

| | Archivo normal (`normal.txt`) | Directorio (`directorio`) | Enlace simbólico (`enlace_normal`) |
|---|---|---|---|
| Tipo | archivo regular | directorio | enlace simbólico |
| Tamaño | 31 bytes | 4096 bytes | 10 bytes |
| Inodo | 2249974 | 2249976 | 2249975 |
| Contador de enlaces | 1 | 2 | 1 |
| Permisos | `rw-rw-r--` (0664) | `rwxrwxr-x` (0775) | `rwxrwxrwx` (0777) |
| Propietario / grupo | `ingesis` / `ingesis` | `ingesis` / `ingesis` | `ingesis` / `ingesis` |

Lectura de permisos por grupos de tres (propietario, grupo, otros):

- `normal.txt` (`rw-` `rw-` `r--`): el propietario y el grupo leen y escriben;
  los demás solo leen.
- `directorio` (`rwx` `rwx` `r-x`): el propietario y el grupo pueden listar,
  crear/borrar entradas y entrar; los demás solo pueden listar y entrar, no
  modificar. En un directorio, la `x` significa poder entrar en él.
- `enlace_normal` (`rwx` `rwx` `rwx`): son los permisos que Linux asigna siempre
  a un enlace simbólico; no se usan, porque al acceder mandan los del destino.

Comentarios y diferencias:

- **Archivo normal:** su tamaño (31 bytes) es el de su contenido y su contador
  de enlaces es 1, porque solo tiene un nombre.
- **Directorio:** también es un archivo: tiene inodo, permisos, propietario y
  fecha de modificación. Sus 4096 bytes no son el tamaño de lo que contiene,
  sino el espacio que ocupa la tabla de entradas (pares *nombre → inodo*), que
  se reserva en bloques de 4096. Su contador de enlaces es 2 aunque esté vacío:
  uno es su nombre (`directorio`) y el otro es su propia entrada `.`. Cada
  subdirectorio que se cree dentro sumaría uno más (por su entrada `..`).
- **Enlace simbólico:** es un archivo distinto del destino, con inodo propio
  (2249975, frente a 2249974 del destino). Su tamaño (10 bytes) es la longitud
  de la ruta que guarda (`normal.txt` tiene 10 caracteres), no los 31 bytes del
  archivo al que apunta. El programa muestra ambos valores por separado para
  no confundirlos: al usar `lstat()` informa el enlace, y con `stat()` consulta
  después el destino (`archivo regular, 31 bytes, inodo 2249974`).

Casos adicionales probados:

- **Enlace duro (`duro.txt`):** el programa lo informa como archivo regular de
  31 bytes con inodo 2249972, con los mismos datos que tenía `archivo.txt`, porque
  es el mismo archivo. Su contador es 1 porque `archivo.txt` ya se había borrado
  en el punto 6.
- **Enlace roto (`simbolico.txt`):** el enlace sigue existiendo (inodo 2249973,
  11 bytes), pero el programa informa `enlace roto (No such file or directory)`
  porque su destino, `archivo.txt`, fue eliminado. Es la misma situación del
  punto 6 vista desde el programa.
- **Archivo inexistente (`no_existe`):** el programa no se cae; muestra un mensaje
  de error con `strerror(errno)` y termina con código 1.