import requests
import sys

url = sys.argv[1]
res = requests.get(url)
print(res.text)
