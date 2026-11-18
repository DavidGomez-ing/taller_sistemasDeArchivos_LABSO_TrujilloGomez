/*
 * infoarchivo.c - Taller de sistemas de archivos (Laboratorio de SO)
 *
 * Uso: ./infoarchivo <archivo>
 *
 * Informa tamano, tipo, permisos, contador de enlaces y fecha de ultima
 * modificacion. Usa lstat(), de modo que si el argumento es un enlace
 * simbolico se informan los metadatos del enlace mismo (no los del
 * destino). En ese caso tambien se muestra a donde apunta y el tamano
 * del destino con stat(), para no confundir ambos tamanos.
 */
#define _XOPEN_SOURCE 700

#include <errno.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

static const char *tipo_archivo(mode_t m)
{
    if (S_ISREG(m))  return "archivo regular";
    if (S_ISDIR(m))  return "directorio";
    if (S_ISLNK(m))  return "enlace simbolico";
    if (S_ISCHR(m))  return "dispositivo de caracteres";
    if (S_ISBLK(m))  return "dispositivo de bloques";
    if (S_ISFIFO(m)) return "FIFO (tuberia con nombre)";
    if (S_ISSOCK(m)) return "socket";
    return "desconocido";
}

/* Construye la cadena tipo "rwxr-xr-x" (9 caracteres + terminador). */
static void cadena_permisos(mode_t m, char out[10])
{
    out[0] = (m & S_IRUSR) ? 'r' : '-';
    out[1] = (m & S_IWUSR) ? 'w' : '-';
    out[2] = (m & S_IXUSR) ? ((m & S_ISUID) ? 's' : 'x') : ((m & S_ISUID) ? 'S' : '-');
    out[3] = (m & S_IRGRP) ? 'r' : '-';
    out[4] = (m & S_IWGRP) ? 'w' : '-';
    out[5] = (m & S_IXGRP) ? ((m & S_ISGID) ? 's' : 'x') : ((m & S_ISGID) ? 'S' : '-');
    out[6] = (m & S_IROTH) ? 'r' : '-';
    out[7] = (m & S_IWOTH) ? 'w' : '-';
    out[8] = (m & S_IXOTH) ? ((m & S_ISVTX) ? 't' : 'x') : ((m & S_ISVTX) ? 'T' : '-');
    out[9] = '\0';
}

static void imprimir_fecha(const char *etiqueta, time_t t)
{
    char buf[64];
    struct tm tmv;

    if (localtime_r(&t, &tmv) != NULL &&
        strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", &tmv) > 0)
        printf("%-22s %s\n", etiqueta, buf);
    else
        printf("%-22s (fecha no disponible)\n", etiqueta);
}

int main(int argc, char *argv[])
{
    struct stat st;
    char perm[10];
    struct passwd *pw;
    struct group *gr;

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <archivo>\n", argv[0]);
        return 1;
    }

    if (lstat(argv[1], &st) == -1) {
        fprintf(stderr, "%s: no se pudo consultar '%s': %s\n",
                argv[0], argv[1], strerror(errno));
        return 1;
    }

    cadena_permisos(st.st_mode, perm);
    pw = getpwuid(st.st_uid);
    gr = getgrgid(st.st_gid);

    printf("Archivo:               %s\n", argv[1]);
    printf("Tipo:                  %s\n", tipo_archivo(st.st_mode));
    printf("Tamano:                %lld bytes\n", (long long)st.st_size);
    printf("Inodo:                 %llu\n", (unsigned long long)st.st_ino);
    printf("Contador de enlaces:   %lu\n", (unsigned long)st.st_nlink);
    printf("Permisos:              %s (octal %04o)\n", perm,
           (unsigned)(st.st_mode & 07777));
    printf("  propietario (%s): %.3s\n", pw ? pw->pw_name : "?", perm);
    printf("  grupo       (%s): %.3s\n", gr ? gr->gr_name : "?", perm + 3);
    printf("  otros:                %.3s\n", perm + 6);
    imprimir_fecha("Ultima modificacion:", st.st_mtime);

    /* Si es un enlace simbolico, mostrar destino y comparar tamanos. */
    if (S_ISLNK(st.st_mode)) {
        char destino[PATH_MAX];
        ssize_t n = readlink(argv[1], destino, sizeof destino - 1);
        struct stat dst;

        if (n == -1) {
            fprintf(stderr, "readlink: %s\n", strerror(errno));
            return 1;
        }
        destino[n] = '\0';
        printf("\nEl enlace apunta a:    %s\n", destino);
        printf("(el tamano del enlace son los %zd caracteres de esa ruta)\n", n);

        if (stat(argv[1], &dst) == -1)
            printf("Destino:               enlace roto (%s)\n", strerror(errno));
        else
            printf("Destino:               %s, %lld bytes, inodo %llu\n",
                   tipo_archivo(dst.st_mode), (long long)dst.st_size,
                   (unsigned long long)dst.st_ino);
    }

    return 0;
}
