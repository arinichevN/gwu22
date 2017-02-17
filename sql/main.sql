
--DROP SCHEMA if exists gwu22 CASCADE;

CREATE SCHEMA gwu22;

CREATE TABLE gwu22.config
(
  app_class character varying (32) NOT NULL,
  db_public character varying (256) NOT NULL,
  udp_port character varying (32) NOT NULL,
  pid_path character varying (32) NOT NULL,
  udp_buf_size character varying (32) NOT NULL, --size of buffer used in sendto() and recvfrom() functions (508 is safe, if more then packet may be lost while transferring over network). Enlarge it if your rio module returns SRV_BUF_OVERFLOW
  db_data character varying (32) NOT NULL,
  CONSTRAINT config_pkey PRIMARY KEY (app_class)
)
WITH (
  OIDS=FALSE
);

CREATE TABLE gwu22.device
(
  app_class character varying(32) NOT NULL,
  pin integer NOT NULL,-- any GPIO pin (use physical address)
  t_id integer NOT NULL,
  h_id integer NOT NULL
  CONSTRAINT device_pkey PRIMARY KEY (app_class, pin)
)
WITH (
  OIDS=FALSE
);



