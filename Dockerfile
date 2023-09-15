FROM python:3.10.5
COPY  .  .
RUN pip install -r req.txt
EXPOSE 5000
CMD [ "main.py" ]