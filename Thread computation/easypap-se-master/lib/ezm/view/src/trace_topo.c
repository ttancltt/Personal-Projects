#include <stdlib.h>
#include <unistd.h>

#include "error.h"
#include "ezv.h"
#include "ezv_topo.h"
#include "trace_graphics.h"
#include "trace_topo.h"

unsigned easyview_show_topo = 1; // 1: enabled, 0: disabled

int trace_topo_load (const char *filename)
{
  char fullpath [PATH_MAX];

  if (filename == NULL)
    exit_with_error ("trace_topo_load: NULL filename");

  if (realpath (filename, fullpath) == NULL) {
      fprintf (stderr,
               "Warning: Cannot access topology file \"%s\".\n"
               " -> Disabling topological view.\n",
               fullpath);
      easyview_show_topo = 0;
      errno = ENOENT;
      return -1;
  }

  if (!ezv_topo_is_initialized ()) {
    // check if fileame exists and is readable
    if (access (fullpath, R_OK) == -1) {
      fprintf (stderr,
               "Warning: Cannot access topology file \"%s\".\n"
               " -> Disabling topological view.\n",
               fullpath);
      easyview_show_topo = 0;
      errno = ENOENT;
      return -1;
    }

    // load topo from file
    if (ezv_topo_init_from_file (fullpath) != 0)
      return -1;
  } else {
    // make sure filename matches topo_file
    char *topo_file = ezv_topo_get_topo_filename ();
    if (topo_file == NULL)
      exit_with_error ("ezv_topo was initialized, but not from a file.");
    
    if (strcmp (topo_file, fullpath) != 0)
      exit_with_error ("Topology file mismatch. Both traces must use the same topology.\n"
                      "  (\"%s\" <> \"%s\")", topo_file,
                       fullpath);
  }

  return 0;
}

static int trace_topo_loaded (void)
{
  return ezv_topo_is_initialized ();
}

void trace_topo_init (void)
{
  if (!trace_topo_loaded ())
    easyview_show_topo = 0;
}

void trace_topo_finalize (void)
{
  // nothing
}

