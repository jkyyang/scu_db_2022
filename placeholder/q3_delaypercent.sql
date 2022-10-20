select companyName, round(delay*100.0/total, 2) as percentage
from
(select s.companyName, count(*) as delay
from 'order' o, 'Shipper' s
where  o.ShipVia = s.Id and 
	ShippedDate > RequiredDate
group by o.ShipVia
)
natural join
(
select s.companyName, count(*) as total
from 'order' o, 'Shipper' s
where  o.ShipVia = s.Id 
group by o.ShipVia
)
order by percentage desc;
