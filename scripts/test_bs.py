import sys
from bs4 import BeautifulSoup as bs

path_prefix="../tests/input/"
suffix=".html"
fn=sys.argv[1]
fp=path_prefix+fn+suffix

html_doc=open(fp, "r").read()
soup=bs(html_doc,"html.parser")

all_a_tags = soup.find_all('a')
for tag in all_a_tags:
	print(tag)

