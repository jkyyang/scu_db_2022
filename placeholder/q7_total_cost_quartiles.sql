select cname, cid, totalcost from
(select cname, cid, totalcost, ntile(4) over (order by totalcost asc) as eq
from
(select 	ifnull(c.CompanyName, 'MISSING_NAME') as cname, 
		o.CustomerId as cid, 
		round(sum(od.UnitPrice * od.Quantity), 2) as totalcost
from 'Order' as o 
	inner join 	OrderDetail as od 
	on			o.Id = od.OrderId
	left join 	Customer as c 	
	on	 		o.CustomerId = c.Id 
group by o.customerId))
where eq = 1
;



