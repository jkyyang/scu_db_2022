select group_concat(fi.pn, ', ') as AnswerString
from(select pd.productname as pn
from 'order' as o, customer as cu, orderdetail as od, product as pd
where o.customerid = cu.id and
	 o.id = od.orderid and
	 od.productid = pd.id and
	 cu.CompanyName = 'Queen Cozinha' and
	 abs(julianday(o.orderdate) -julianday('2014-12-25'))< 1
order by pd.id) as fi;
	