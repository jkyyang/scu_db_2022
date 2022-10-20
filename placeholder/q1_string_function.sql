select distinct ShipName, substr(ShipName, 0, instr(ShipName, '-')) as Answer
from 'Order'
where ShipName like '%-%'
order by ShipName
;

