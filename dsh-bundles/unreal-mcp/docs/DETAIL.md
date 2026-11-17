# Unreal Engine MCP

Connects to the MCP server that the Unreal Editor hosts inside its own process
(the experimental `ModelContextProtocol` / "Unreal MCP" plugin, UE 5.8+).

- Endpoint: `http://127.0.0.1:8000/mcp` (Streamable HTTP, localhost only).
- Start it either from **Project Settings → Plugins → Model Context Protocol →
  Auto Start Server**, or by launching the editor with
  `-ModelContextProtocolStartServer` (and optionally `-ModelContextProtocolPort=N`).
- The server exposes the toolset registry. With tool search enabled (default)
  `tools/list` returns three meta-tools — `list_toolsets`, `describe_toolset`,
  `call_tool` — and individual toolset tools are invoked through `call_tool`.

If the editor is closed the connection has nothing to talk to; start the editor
before using these tools. This bundle only stores the endpoint, no credentials.
