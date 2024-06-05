add_library(
        redbase_pf
        OBJECT
        pf_manager.cpp
)

set(ALL_OBJECT_FILES
        ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:redbase_pf>
        PARENT_SCOPE)