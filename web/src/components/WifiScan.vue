<template>
  <n-button>naive-ui</n-button>
  
  <n-data-table
    :bordered="false"
    :columns="columns"
    :data="data"
    :pagination="pagination"
  />
</template>

<script setup lang="ts">
import { h } from 'vue'
import { NButton, NDataTable, NTag } from 'naive-ui'
    
type RowData = {
  key: number
  name: string
  age: number
  address: string
  tags: string[]
}

const connect = (row: RowData) => {
  console.log('Send email to', row.name)
}

const columns = [
    {
      title: 'Name',
      key: 'name'
    },
    {
      title: 'Age',
      key: 'age'
    },
    {
      title: 'Address',
      key: 'address'
    },
    {
      title: 'Tags',
      key: 'tags',
      render (row: RowData) {
        const tags = row.tags.map((tagKey) => {
          return h(
            NTag,
            {
              style: {
                marginRight: '6px'
              },
              type: 'info',
              bordered: false
            },
            {
              default: () => tagKey
            }
          )
        })
        return tags
      }
    },
    {
      title: 'Action',
      key: 'actions',
      render (row: RowData) {
        return h(
          NButton,
          {
            size: 'small',
            onClick: () => connect(row)
          },
          { default: () => 'Connect' }
        )
      }
    }
  ]

const data: RowData[] =  [
  {
    key: 0,
    name: 'John Brown',
    age: 32,
    address: 'New York No. 1 Lake Park',
    tags: ['nice', 'developer']
  },
  {
    key: 1,
    name: 'Jim Green',
    age: 42,
    address: 'London No. 1 Lake Park',
    tags: ['wow']
  },
  {
    key: 2,
    name: 'Joe Black',
    age: 32,
    address: 'Sidney No. 1 Lake Park',
    tags: ['cool', 'teacher']
  }
]
const pagination = {
  pageSize: 10
}
</script>