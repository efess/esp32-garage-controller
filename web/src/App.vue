<template>
  
  <n-config-provider :theme="darkTheme">
    <header>
    </header>

    <main>
      <WifiScan />
    </main>
  </n-config-provider>
</template>

<script setup lang="ts">
import { darkTheme, NConfigProvider } from 'naive-ui'
import WifiScan from './components/WifiScan.vue'
</script>

<style scoped>
header {
  line-height: 1.5;
}

.logo {
  display: block;
  margin: 0 auto 2rem;
}

@media (min-width: 1024px) {
  header {
    display: flex;
    place-items: center;
    padding-right: calc(var(--section-gap) / 2);
  }

  .logo {
    margin: 0 2rem 0 0;
  }

  header .wrapper {
    display: flex;
    place-items: flex-start;
    flex-wrap: wrap;
  }
}
</style>
