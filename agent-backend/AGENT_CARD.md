# DrivePilot Agent Card

## Purpose

Translate natural-language requests into a controlled sequence of simulated cockpit operations.

## Inputs

- typed text;
- final text produced by XFYUN ASR;
- previous conversation messages;
- Qt tool results.

## Outputs

- `analysis`: public execution-stage summary;
- `plan`: tool plan;
- `tool_call`: structured Qt operation;
- `observation`: actual execution result;
- `final`: natural-language response.

## Tool Set

- read vehicle state;
- open page;
- set AC temperature, fan and mode;
- control music and volume;
- toggle selected simulated switches;
- show Toast.

## Controls

- tool names are whitelisted;
- arguments use JSON Schema;
- Qt validates again;
- one `call_id` maps one result;
- tool timeout: 20 s by default;
- maximum 8 model turns;
- tasks can be cancelled;
- session can be reset;
- model failure can fall back to local demo.

## Data and Privacy

User text may be sent to the configured cloud model. Camera frames are never sent to Agent. Secrets remain in `.env`.

## Limitations

- no RAG in v1.0;
- no long-term memory;
- no real vehicle hardware;
- model responses can be wrong;
- no safety-critical tool is exposed;
- cloud latency and cost depend on provider.
