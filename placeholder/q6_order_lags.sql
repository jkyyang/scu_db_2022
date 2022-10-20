select id, orderdate, preorderdate as PreviousOrderDate, round(julianday(orderdate) - julianday(preorderdate), 2) as SubAnswer

from(
select id, OrderDate, lag(orderdate, 1, orderdate) over (order by orderdate asc) as preorderdate
from 'order'
where CustomerId = 'BLONP'
order by orderDate
limit 10
)
;
