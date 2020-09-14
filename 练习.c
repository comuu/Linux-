link *iteration_reverse(link *head)
{
	if(head == NULL || head->next == NULL)
		return -1;
	else
	link *beg = NULL;
	link *mid = head;
	link *end = head->next;
	while(end != NULL)
	{
		mid->next = beg;
		beg = mid;
		mid = end;
		eng = end->next;
	}
	head = mid;
	return head;










}