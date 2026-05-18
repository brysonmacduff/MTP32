# MTP32 — Minimal 32‑Byte Request–Reply Protocol

MTP32 is a lightweight, single‑packet message protocol designed for half‑duplex radios such as the nRF24L01+. It provides a simple master–slave request–reply pattern with UDP‑like semantics:

No acknowledgements

No retransmissions

No message chunking

No session state

No role switching

The protocol is intentionally minimal, making it ideal for embedded systems with tight timing and memory constraints.

## Protocol Overview

MTP32 defines a fixed‑size 32‑byte packet, including a small header and an application payload. Each packet is self‑contained; there is no multi‑packet message assembly.

Communication always follows a request–reply cycle:

Master transmits a request

Slave receives the request

Slave transmits a reply

Master receives the reply

If a packet is lost, corrupted, or never arrives, the protocol does not retry automatically. Higher‑level logic may choose to send another request later.

## Roles

### Master

Initiates all communication

Sends requests

Waits for replies

If a reply does not arrive before timeout, the master returns to TRANSMIT to send the next request

### Slave

Never initiates communication

Waits for requests

Sends replies only after receiving a request

If no request arrives before timeout, the slave remains in RECEIVE

This asymmetry ensures the system never deadlocks with both radios stuck in RECEIVE.

## State Machine Summary

MTP32 uses two top‑level states:

TRANSMIT — sending a request (master) or reply (slave)

RECEIVE — waiting for a packet

The master and slave share the same state machine, but differ in how they handle RX timeouts.

### Master Timeout Behavior

If the master times out while waiting for a reply, it transitions back to TRANSMIT to send the next request.

### Slave Timeout Behavior

If the slave times out while waiting for a request, it stays in RECEIVE.

This guarantees:

The master always drives progress

The slave never transmits spontaneously

No collisions

No deadlock
