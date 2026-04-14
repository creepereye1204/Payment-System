FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    gcc \
    make \
    libmysqlclient-dev \
    default-mysql-client \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN make server

EXPOSE 8080

CMD ["./server"]
