FROM python:3
ENV PYTHONUNBUFFERED 1
RUN mkdir /ups_web
WORKDIR /ups_web
ADD requirements.txt /ups_web
RUN pip install -r requirements.txt
ADD . /ups_web
RUN chmod +x initserver.sh
RUN chmod +x runserver.sh