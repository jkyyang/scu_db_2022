select id, shipcountry, 
case shipcountry 
	when 'USA' then 'NorthAmerica' 
	when 'Mexico' then 'NorthAmerica' 
	when 'Canada' then 'NorthAmerica' 
	else 'OtherPlace' 
end as Region

from 'Order'
where Id >= 15445
order by id
limit 20
;