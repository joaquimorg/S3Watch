idf_component_register(
  SRCS
    "test_nimble.c"
    "test_buffer.c"
  REQUIRES
    unity
    nimble-nordic-uart
)