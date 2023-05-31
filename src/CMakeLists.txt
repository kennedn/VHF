set(CMAKE_BUILD_TYPE Debug)


add_executable(vhf)

pico_generate_pio_header(vhf ${CMAKE_CURRENT_LIST_DIR}/pal.pio)
# pico_generate_pio_header(vhf ${CMAKE_CURRENT_LIST_DIR}/pal_data.pio)

target_compile_definitions(vhf PRIVATE)
target_sources(vhf PRIVATE vhf.c pal.c)

target_include_directories(vhf PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(vhf PRIVATE 
    pico_stdlib 
    hardware_pio
    hardware_dma
)
pico_add_extra_outputs(vhf)
