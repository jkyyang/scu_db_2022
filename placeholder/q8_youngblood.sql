select reg.regiondescription as RegionDescription, e.firstname as FirstName, e.lastname as LastName, max(e.birthdate) as LatestDate
from 
(
select et.employeeid as eid, t.regionid as regid
from EmployeeTerritory et, Territory t 
where et.territoryid = t.id
) as enr, employee e, region reg
where enr.eid = e.id and
	 enr.regid = reg.id
group by enr.regid
order by enr.regid asc
;