select pName, CompanyName, ContactName
from 
(select pName, CustomerID, min(OrderDate)
from
(	select dt.ProductName as pName, od.OrderID as orderid
	from 
	orderdetail as od, 
	(
		select id, ProductName
		from product 
		where discontinued = 1
	) as dt
	where od.productid = dt.id
) as op, 
'Order' as odr
where op.orderid = odr.id
group by pname
) as pre, Customer
where pre.CustomerID = customer.id
order by pName;