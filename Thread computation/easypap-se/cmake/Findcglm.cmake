# Findcglm.cmake
#
# Ce module définit les cibles importées suivantes :
#
#   cglm::cglm - La bibliothèque cglm si elle est trouvée

# Inclure les outils de gestion des packages
include(FindPackageHandleStandardArgs)

# Rechercher les fichiers d'en-tête et la bibliothèque
find_path(CGLM_INCLUDE_DIR
    NAMES cglm/cglm.h
    HINTS ${CGLM_ROOT_DIR}/include
)

find_library(CGLM_LIBRARY
    NAMES cglm
    HINTS ${CGLM_ROOT_DIR}/lib
)

# Gérer les arguments standard de FindPackage
find_package_handle_standard_args(cglm
    REQUIRED_VARS CGLM_LIBRARY CGLM_INCLUDE_DIR
)

if(CGLM_FOUND)
    # Créer une cible importée pour cglm
    add_library(cglm::cglm UNKNOWN IMPORTED)
    set_target_properties(cglm::cglm PROPERTIES
        IMPORTED_LOCATION ${CGLM_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${CGLM_INCLUDE_DIR}
    )
endif()

# Définir les variables pour la compatibilité ascendante
set(CGLM_LIBRARIES ${CGLM_LIBRARY})
set(CGLM_INCLUDE_DIRS ${CGLM_INCLUDE_DIR})

# Afficher un message de statut
if(CGLM_FOUND)
    message(STATUS "Found cglm: ${CGLM_LIBRARY}")
else()
    message(STATUS "cglm not found.")
endif()

mark_as_advanced(CGLM_INCLUDE_DIR CGLM_LIBRARY)
