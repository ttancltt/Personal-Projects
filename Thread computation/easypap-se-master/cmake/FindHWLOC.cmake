# FindHwloc.cmake
# ---------------
#
# Trouve la bibliothèque Hwloc (Portable Hardware Locality) via pkg-config
#
# Ce module définit les variables suivantes :
#   HWLOC_FOUND        - TRUE si hwloc a été trouvé
#   HWLOC_VERSION      - Version de hwloc
#   HWLOC_INCLUDE_DIRS - Répertoires d'en-têtes de hwloc
#   HWLOC_LIBRARIES    - Bibliothèques hwloc à lier
#
# Ce module définit la cible suivante :
#   HWLOC::HWLOC       - Cible importée pour hwloc

# Utilise pkg-config pour obtenir les informations sur hwloc
find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_HWLOC REQUIRED hwloc)

# Définit les variables de sortie
set(HWLOC_FOUND ${PC_HWLOC_FOUND})
set(HWLOC_VERSION ${PC_HWLOC_VERSION})
set(HWLOC_INCLUDE_DIRS ${PC_HWLOC_INCLUDE_DIRS})
set(HWLOC_LIBRARIES ${PC_HWLOC_LIBRARIES})

# Crée la cible importée HWLOC::HWLOC si elle n'existe pas déjà
if(HWLOC_FOUND AND NOT TARGET HWLOC::HWLOC)
    add_library(HWLOC::HWLOC INTERFACE IMPORTED)
    set_target_properties(HWLOC::HWLOC PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${PC_HWLOC_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${PC_HWLOC_LIBRARIES}"
        INTERFACE_LINK_DIRECTORIES "${PC_HWLOC_LIBRARY_DIRS}"
    )
    
    # Ajoute les flags de compilation si disponibles
    if(PC_HWLOC_CFLAGS_OTHER)
        set_target_properties(HWLOC::HWLOC PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${PC_HWLOC_CFLAGS_OTHER}"
        )
    endif()
    
    # Ajoute les options de linkage si disponibles
    if(PC_HWLOC_LDFLAGS_OTHER)
        set_target_properties(HWLOC::HWLOC PROPERTIES
            INTERFACE_LINK_OPTIONS "${PC_HWLOC_LDFLAGS_OTHER}"
        )
    endif()
endif()
