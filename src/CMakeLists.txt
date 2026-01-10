add_executable(client)
target_sources(client PRIVATE client.c game.c)

add_executable(server)
target_sources(server PRIVATE server.c game.c)