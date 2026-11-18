#!/usr/bin/env bash
# Ejecuta los comandos del taller y guarda la salida en evidencia/evidencia.txt
# para copiar los valores reales a la bitacora.
# Uso: ./correr_taller.sh
set -u

cd "$(dirname "$0")" || exit 1
make -s || { echo "Fallo la compilacion"; exit 1; }

OUT="$PWD/evidencia/evidencia.txt"
PRUEBAS=pruebas
mkdir -p evidencia
rm -rf "$PRUEBAS"; mkdir "$PRUEBAS"
: > "$OUT"

seccion() { printf '\n==================== %s ====================\n' "$1" | tee -a "$OUT"; }
ejecutar() { printf '\n$ %s\n' "$*" | tee -a "$OUT"; "$@" 2>&1 | tee -a "$OUT"; }
# Igual que ejecutar, pero para comandos que se espera que fallen
ejecutar_sh() { printf '\n$ %s\n' "$1" | tee -a "$OUT"; bash -c "$1" 2>&1 | tee -a "$OUT"; }

seccion "PARTE 1: sistemas de archivos montados"
ejecutar df -h
ejecutar df -i
ejecutar lsblk -f

cd "$PRUEBAS" || exit 1
echo "contenido de prueba del taller" > archivo.txt

seccion "PARTE 1 (punto 3): metadatos de archivo y directorio"
ejecutar stat archivo.txt
ejecutar stat .
ejecutar ls -li

seccion "PARTE 2 (puntos 4 y 5): enlaces"
ejecutar ln archivo.txt duro.txt
ejecutar ln -s archivo.txt simbolico.txt
ejecutar ls -li archivo.txt duro.txt simbolico.txt
ejecutar stat -c '%n  inodo=%i  enlaces=%h  tamano=%s  tipo=%F' archivo.txt duro.txt simbolico.txt

seccion "PARTE 2 (punto 6): borrar el original"
ejecutar rm archivo.txt
ejecutar ls -li duro.txt simbolico.txt
ejecutar_sh 'cat duro.txt'
ejecutar_sh 'cat simbolico.txt'
ejecutar stat -c '%n  inodo=%i  enlaces=%h' duro.txt

seccion "PARTE 2 (punto 7): enlaces entre sistemas de archivos"
# /dev/shm es tmpfs, es decir, otro sistema de archivos distinto del de la carpeta actual
ejecutar df --output=source,fstype,target . /dev/shm
ejecutar_sh 'ln duro.txt /dev/shm/duro_otro_fs.txt'
ejecutar_sh 'ln -s "$PWD/duro.txt" /dev/shm/simbolico_otro_fs.txt && ls -li /dev/shm/simbolico_otro_fs.txt && cat /dev/shm/simbolico_otro_fs.txt'
rm -f /dev/shm/simbolico_otro_fs.txt

cd ..
seccion "PARTE 3 (punto 9): programa sobre archivo, directorio y enlace"
cd "$PRUEBAS" || exit 1
echo "archivo nuevo para el programa" > normal.txt
ln -s normal.txt enlace_normal
mkdir directorio
ejecutar ../infoarchivo normal.txt
ejecutar ../infoarchivo directorio
ejecutar ../infoarchivo enlace_normal
ejecutar ../infoarchivo duro.txt
ejecutar ../infoarchivo simbolico.txt
ejecutar ../infoarchivo no_existe

echo
echo "Listo. Evidencia guardada en $OUT"
