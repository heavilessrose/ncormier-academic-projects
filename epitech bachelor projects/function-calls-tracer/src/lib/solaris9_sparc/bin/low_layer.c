/*
** low_layer.c for  in /u/ept3/cormie_n/lang/c/ftrace/src/lib/solaris9_sparc/bin
**
** Made by nicolas cormier
** Login   <cormie_n@epita.fr>
**
** Started on  Wed Apr 12 21:37:38 2006 nicolas cormier
** Last update Wed Apr 12 21:38:01 2006 nicolas cormier
*/

#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "elfobj.h"

#include "includes/error.h"

#include "api/includes/prototypes/bin/low_layer.h"


bin_obj_t*	bin_new(char* path, addr_t base)
{
  struct stat	sb;
  int		fd;
  void*		data;
  bin_obj_t*	ret;
  elfobj_t*	luse;
  int		i = 0;
  char*		ptr;

  /*
  ** Map the binary.
  */
  if ((fd = open(path, O_RDONLY)) == -1)
    {
      return (NULL);
    }
  if (fstat(fd, &sb) == -1)
    {
      close(fd);
      return (NULL);
    }
  data = mmap(0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  if (data == MAP_FAILED)
    return (NULL);
  /*
  ** Check if the binary is really a elf.
  */
  if (!IS_ELF(*((Elf_Ehdr*)data)))
    {
      errno = EFTRACE;
      ftrace_errstr = "File is not an elf file.";
      munmap(data, sb.st_size);
      return (NULL);
    }
  /*
  ** Fill the return
  */
  ret = malloc(sizeof(bin_obj_t));
  if (ret == NULL)
    {
      munmap(data, sb.st_size);
      return (NULL);
    }
  ret->path = strdup(path);
  ret->map = data;
  ret->fd = -1;
  ret->size = sb.st_size;
  ret->luse = NULL;
  /*
  ** Fill luse.
  */
  luse = malloc(sizeof(elfobj_t));
  if (luse == NULL)
    return (ret);
  luse->base = base;
  luse->header = data;
  luse->program_headers = NULL;
  luse->section_headers = NULL;
  luse->section_str = NULL;
  /*
  ** Program headers.
  */
  luse->program_headers = malloc(sizeof(Elf_Phdr *) * (luse->header->e_phnum + 1));
  if (luse->program_headers == NULL)
    goto luse_fill_failed;
  for (ptr = (char*) luse->header + luse->header->e_phoff, i = 0; i < luse->header->e_phnum; i++)
    {
      luse->program_headers[i] = (Elf_Phdr *) ptr;
      ptr += luse->header->e_phentsize;
    }
  luse->program_headers[i] = NULL;
  /*
  ** Section headers.
  */
  luse->section_headers = malloc(sizeof(Elf_Shdr *) * (luse->header->e_shnum + 1));
  if (luse->section_headers == NULL)
    goto luse_fill_failed;
  for (ptr = (char*) luse->header + luse->header->e_shoff, i = 0; i < luse->header->e_shnum; i++)
    {
      luse->section_headers[i] = (Elf_Shdr *) ptr;
      ptr += luse->header->e_shentsize;
    }
  luse->section_headers[i] = NULL;
  /*
  ** Section strng.
  */
  if (luse->header->e_shstrndx != SHN_UNDEF)
    luse->section_str = data + luse->section_headers[luse->header->e_shstrndx]->sh_offset;
  else
    luse->section_str = NULL;
  ret->luse = luse;
  return (ret);
 luse_fill_failed:
  bin_del(ret);
  if (luse->program_headers)
    free(luse->program_headers);
  if (luse->section_headers)
    free(luse->section_headers);
  free(luse);
  return (NULL);
}

int	bin_del(bin_obj_t* obj)
{
  elfobj_t*	luse;

  if (obj->path)
    free(obj->path);
  if (obj->map)
    {
      if (munmap(obj->map, obj->size) == -1)
	return (-1);
    }
  if (obj->fd == -1)
    {
      if (close(obj->fd) == -1)
	return (-1);
    }
  if (obj->luse)
    {
      luse = obj->luse;
      if (luse->program_headers)
	free(luse->program_headers);
      if (luse->section_headers)
	free(luse->section_headers);
      free(luse);
    }
  free(obj);
  return (0);
}

int	bin_read(bin_obj_t* obj, addr_t addr, unsigned int len, char* laddr)
{
  memcpy(laddr, obj->map + addr, len);
  return (0);
}


