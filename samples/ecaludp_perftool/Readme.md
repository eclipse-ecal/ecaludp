# ecaludp-perftool

ecaludp_perftool is a simple tool for sending and receiving data using the eCAL protocol.

```
Usage:
  ecaludp_perftool <IMPLEMENTATION> [PARAMETERS]
  
With IMPLEMENTATION one of:
  send                Asio-based sender using send_to in a while-loop
  sendasync           Asio-based sender using async_send_to
  receive             Asio-based receiver using receive_from in a while-loop
  receiveasync        Asio-based receiver using async_receive_from
  receivenpcap        Npcap-based receiver using receive_from in a while-loop
  receivenpcapasync   Npcap-based receiver using async_receive_from

Options:
  -h, --help  Show this help message and exit

  -i, --ip <IP> IP address to send to / receive from. Default to 127.0.0.1
  -p, --port <PORT> Port to send to / receive from. Default to 14000
  -s, --size <SIZE> Message size to send. Default to 0 (-> empty messages)
  -m, --max-udp-datagram-size <SIZE> Maximum UDP datagram size
  -b, --buffer-size <SIZE> Buffer size for sending & receiving messages
```