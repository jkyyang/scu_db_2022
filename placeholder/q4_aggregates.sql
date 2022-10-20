select c.categoryName, count(*) as totalNumber, round(avg(p.unitPrice), 2) as average, min(p.unitPrice) as minimum, max(p.unitPrice) as maximun, sum(p.unitsonorder) as totalUnitsOnOrder
from Product as p join Category as c
on c.id = p.categoryid
group by p.categoryid
having totalNumber > 10
order by c.id
;